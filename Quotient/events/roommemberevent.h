// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Karol Kosek <krkkx@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"
#include <Quotient/quotient_common.h>

namespace Quotient {
class QUOTIENT_API MemberEventContent {
public:
    Q_IMPLICIT MemberEventContent(Membership ms) : membership(ms) {}
    explicit MemberEventContent(const QJsonObject& json);
    QJsonObject toJson() const;

    Membership membership;
    /// (Only for invites) Whether the invite is to a direct chat
    bool isDirect = false;
    std::optional<QString> displayName;
    std::optional<QUrl> avatarUrl;
    QString reason;
};

class QUOTIENT_API RoomMemberEvent
    : public KeyedStateEventBase<RoomMemberEvent, MemberEventContent> {
    Q_GADGET
public:
    QUO_EVENT(RoomMemberEvent, "m.room.member")

    static bool isValid(const QJsonObject& fullJson)
    {
        return !fullJson[StateKeyKey].toString().isEmpty();
    }

    using KeyedStateEventBase::KeyedStateEventBase;

    QUO_CONTENT_GETTER(Membership, membership)
    // Membership membership() const { return content().membership; }
    QString userId() const { return stateKey(); }
    QUO_CONTENT_GETTER(bool, isDirect)
    // bool isDirect() const { return content().isDirect; }
    QUO_CONTENT_GETTER_X(std::optional<QString>, newDisplayName, "displayname"_L1)
    // std::optional<QString> newDisplayName() const { return content().displayName; }
    QUO_CONTENT_GETTER_X(std::optional<QUrl>, newAvatarUrl, "avatar_url"_L1)
    // std::optional<QUrl> newAvatarUrl() const { return content().avatarUrl; }
    QUO_CONTENT_GETTER(QString, reason)
    // QString reason() const { return content().reason; }
    bool changesMembership() const;
    bool isBan() const;
    bool isUnban() const;
    bool isInvite() const;
    bool isRejectedInvite() const;
    bool isJoin() const;
    bool isLeave() const;
    bool isRename() const;
    bool isAvatarUpdate() const;
};
} // namespace Quotient
