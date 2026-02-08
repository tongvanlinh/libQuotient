// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Karol Kosek <krkkx@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommemberevent.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

MemberEventContent::MemberEventContent(const QJsonObject& json)
    : membership(fromJson<Membership>(json["membership"_L1]))
    , isDirect(json["is_direct"_L1].toBool())
    , displayName(fromJson<std::optional<QString>>(json["displayname"_L1]))
    , avatarUrl(fromJson<std::optional<QString>>(json["avatar_url"_L1]))
    , reason(json["reason"_L1].toString())
{
    if (displayName)
        displayName = sanitized(*displayName);
}

QJsonObject MemberEventContent::toJson() const
{
    QJsonObject o;
    if (membership != Membership::Invalid)
        o.insert("membership"_L1, Quotient::toJson(membership));
    if (displayName)
        o.insert("displayname"_L1, *displayName);
    if (avatarUrl && avatarUrl->isValid())
        o.insert("avatar_url"_L1, avatarUrl->toString());
    if (!reason.isEmpty())
        o.insert("reason"_L1, reason);
    return o;
}

bool RoomMemberEvent::changesMembership() const
{
    return !prevContent() || prevContent()->membership != membership();
}

bool RoomMemberEvent::isInvite() const
{
    return membership() == Membership::Invite && changesMembership();
}

bool RoomMemberEvent::isRejectedInvite() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership == Membership::Invite;
}

bool RoomMemberEvent::isJoin() const
{
    return membership() == Membership::Join && changesMembership();
}

bool RoomMemberEvent::isLeave() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership != membership()
           && prevContent()->membership != Membership::Ban
           && prevContent()->membership != Membership::Invite;
}

bool RoomMemberEvent::isBan() const
{
    return membership() == Membership::Ban && changesMembership();
}

bool RoomMemberEvent::isUnban() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership == Membership::Ban;
}

bool RoomMemberEvent::isRename() const
{
    return prevContent() && prevContent()->displayName
               ? newDisplayName() != *prevContent()->displayName
               : newDisplayName().has_value();
}

bool RoomMemberEvent::isAvatarUpdate() const
{
    return prevContent() && prevContent()->avatarUrl
               ? newAvatarUrl() != *prevContent()->avatarUrl
               : newAvatarUrl().has_value();
}

QString RoomMemberEvent::bestEffortDisplayName(bool fallbackToMxid) const
{
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    auto displayName = newDisplayName();
    if (!displayName)
        displayName = lift(&MemberEventContent::displayName, prevContent());
    return lift(&sanitized, displayName).value_or(fallbackToMxid ? userId() : QString());
}

QString RoomMemberEvent::fullName() const
{
    const auto displayName = bestEffortDisplayName();
    return displayName.isEmpty() ? id() : displayName % u" (" % id() % u')';
}

bool RoomMemberEvent::fullNameMatches(QStringView substr, Qt::CaseSensitivity cs) const
{
    return fullName().contains(substr, cs);
}
