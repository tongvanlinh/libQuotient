// SPDX-FileCopyrightText: 2023 James Graham <james.h.graham@protonmail.com>
// SPDX-FileCopyrightText: 2026 Alexey Rusakov <kitsune@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "memberproxy.h"

#include "logging_categories_p.h"
#include "room.h"
#include "util.h"
#include "events/roommemberevent.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>

using namespace Quotient;

MemberProxy::MemberProxy(const Room *room, const RoomMemberEvent *member)
    : _room(room), _member(member)
{
    if (member) {
        _memberId = member->userId();
        _hueF = stringToHueF(_memberId);
        _roomListener = QObject::connect(_room, &Room::memberListChanged, [this] {
            auto currentMember = _room->member(_memberId);
            if (currentMember._member == _member) // nullptr or not
                return;
            if (!currentMember._member)
                qCritical(EVENTS) << "The MemberProxy object for user" << _memberId
                                  << "is about to dangle";
            else
                qWarning(EVENTS) << "Avoided MemberProxy object dangling for user" << _memberId;

            qWarning(EVENTS) << "Do not store MemberProxy objects, they are not long-lived";
            _member = currentMember._member;
        });
    }
}

MemberProxy::~MemberProxy() { QObject::disconnect(_roomListener); }

bool MemberProxy::operator==(const MemberProxy& other) const { return id() == other.id(); }

QString MemberProxy::id() const { return _memberId; }

const RoomMemberEvent *MemberProxy::event() const { return _member; }

Uri MemberProxy::uri() const { return Uri(id().toLatin1()); }

bool MemberProxy::isLocalMember() const
{
    if (_room == nullptr) {
        return false;
    }
    return id() == _room->localUserId();
}

Membership MemberProxy::membershipState() const
{
    if (_member == nullptr) {
        return Membership::Undefined;
    }
    return _member->membership();
}

QString MemberProxy::name() const
{
    return _member ? _member->bestEffortDisplayName() : QString();
}

QString MemberProxy::displayName() const
{
    if (auto dispName = name(); !dispName.isEmpty())
        return name();
    return id();
}

QString MemberProxy::htmlSafeDisplayName() const { return displayName().toHtmlEscaped(); }

QString MemberProxy::fullName() const { return _member ? _member->fullName() : QString(); }

QString MemberProxy::htmlSafeFullName() const { return fullName().toHtmlEscaped(); }

QString MemberProxy::disambiguatedName() const
{
    return _room->needsDisambiguation(id()) ? fullName() : displayName();
}

QString MemberProxy::htmlSafeDisambiguatedName() const
{
    return disambiguatedName().toHtmlEscaped();
}

bool MemberProxy::matches(QStringView substr, Qt::CaseSensitivity cs) const
{
    return _member && _member->fullNameMatches(substr, cs);
}

int MemberProxy::hue() const { return static_cast<int>(hueF() * 359); }

qreal MemberProxy::hueF() const { return _hueF; }

QColor MemberProxy::color() const
{
    const auto lightness = QGuiApplication::palette().color(QPalette::Active, QPalette::Window).lightnessF();
    // https://github.com/quotient-im/libQuotient/wiki/User-color-coding-standard-draft-proposal
    return QColor::fromHslF(static_cast<float>(hueF()), 1.0f, -0.7f * lightness + 0.9f, 1.0f);
}

const Avatar& MemberProxy::avatarObject() const
{
    return _room->connection()->userAvatar(avatarUrl());
}

namespace {
QUrl getMediaId(const RoomMemberEvent* evt)
{
    // See https://github.com/matrix-org/matrix-spec/issues/322
    QUrl baseUrl;
    if (evt->newAvatarUrl())
        baseUrl = *evt->newAvatarUrl();
    else if (evt->prevContent() && evt->prevContent()->avatarUrl)
        baseUrl = *evt->prevContent()->avatarUrl;

    return Avatar::isUrlValid(baseUrl) ? baseUrl : QUrl();
}
}

QString MemberProxy::avatarMediaId() const
{
    return isEmpty() ? QString() : getMediaId(_member).toString();
}

QUrl MemberProxy::avatarUrl() const {
    if (isEmpty())
        return {};

    const auto mediaId = getMediaId(_member);
    return mediaId.isValid() ? _room->connection()->makeMediaUrl(mediaId) : QUrl();
}

int MemberProxy::powerLevel() const
{
    if (_room == nullptr || _member == nullptr) {
        return std::numeric_limits<int>::min();
    }
    return _room->memberEffectivePowerLevel(id());
}

QImage MemberProxy::avatar(int width, int height, Avatar::get_callback_t callback) const
{
    return avatarObject().get(width, height, std::move(callback));
}

QImage MemberProxy::avatar(int dimension, Avatar::get_callback_t callback) const
{
    return avatar(dimension, dimension, std::move(callback));
}

namespace {
inline QStringView removeLeadingAt(QStringView sv) { return sv.mid(sv.startsWith(u'@') ? 1 : 0); }
}

bool MemberSorter::operator()(QStringView u1name, QStringView u2name) const
{
    return removeLeadingAt(u1name).localeAwareCompare(removeLeadingAt(u2name)) < 0;
}

bool MemberSorter::operator()(const RoomMemberEvent *e1, const RoomMemberEvent *e2) const
{
    return operator()(e1->bestEffortDisplayName(), e2->bestEffortDisplayName());
}
