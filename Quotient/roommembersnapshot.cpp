// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "roommembersnapshot.h"

#include "room.h"
#include "util.h"
#include "events/roommemberevent.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>

using namespace Quotient;

RoomMemberSnapshot::RoomMemberSnapshot(const Room *room, const RoomMemberEvent *member)
    : _room(member ? room : nullptr)
    , _memberId(member ? member->userId() : QString())
    , _displayName(member ? member->bestEffortDisplayName() : QString())
    , _avatarMediaId(member ? member->bestEffortAvatarUrl() : QUrl())
    , _hueF(member ? stringToHueF(member->userId()) : 0.0)
    , _powerLevel(member && room ? room->memberEffectivePowerLevel(member->userId())
                                 : std::numeric_limits<int>::min())
    , _membership(member ? member->membership() : Membership::Undefined)
{}

Uri RoomMemberSnapshot::uri() const { return Uri(id().toLatin1()); }

bool RoomMemberSnapshot::isLocalMember() const { return _room && id() == _room->localUserId(); }

QString RoomMemberSnapshot::displayName() const
{
    return !_displayName.isEmpty() ? _displayName : id();
}

QString RoomMemberSnapshot::fullName() const
{
    if (_displayName.isEmpty())
        return id();
    return _displayName % u" (" % _memberId % u')';
}

QString RoomMemberSnapshot::disambiguatedName() const
{
    return _room && _room->needsDisambiguation(id()) ? fullName() : displayName();
}

bool RoomMemberSnapshot::matches(QStringView substr, Qt::CaseSensitivity cs) const
{
    return fullName().contains(substr, cs);
}

QColor RoomMemberSnapshot::color() const
{
    const auto lightness =
        QGuiApplication::palette().color(QPalette::Active, QPalette::Window).lightnessF();
    return QColor::fromHslF(static_cast<float>(hueF()), 1.0f, -0.7f * lightness + 0.9f, 1.0f);
}

const Avatar &RoomMemberSnapshot::avatarObject() const
{
    QUO_CHECK(_room);
    return _room->connection()->userAvatar(avatarUrl());
}

QUrl RoomMemberSnapshot::avatarUrl() const
{
    return isEmpty() || !_avatarMediaId.isValid()
               ? QUrl()
               : _room->connection()->makeMediaUrl(_avatarMediaId);
}

QImage RoomMemberSnapshot::avatar(int width, int height, Avatar::get_callback_t callback) const
{
    return avatarObject().get(width, height, std::move(callback));
}

QImage RoomMemberSnapshot::avatar(int dimension, Avatar::get_callback_t callback) const
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
