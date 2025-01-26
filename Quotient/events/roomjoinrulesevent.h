// SPDX-FileCopyrightText: 2025 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/events/stateevent.h>

namespace Quotient
{
namespace EventContent {
Q_NAMESPACE_EXPORT(QUOTIENT_API)

//! \brief Definition of an allow AllowCondition
//!
//! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_ruless
struct AllowCondition {
    QString roomId;
    QString type;
};

//! Enum representing the available room join rules
enum JoinRule {
    Public,
    Knock,
    Invite,
    Private,
    Restricted,
    KnockRestricted,
};
Q_ENUM_NS(JoinRule)

[[maybe_unused]] constexpr std::array JoinRuleStrings {
    "public"_L1,
    "knock"_L1,
    "invite"_L1,
    "private"_L1,
    "restricted"_L1,
    "knock_restricted"_L1,
};

//! \brief The content of a join rule event
//!
//! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
struct JoinRuleContent {
    JoinRule joinRule;
    QList<AllowCondition> allow;
};
} // namespace EventContent

template<>
inline EventContent::AllowCondition fromJson(const QJsonObject& jo)
{
    return EventContent::AllowCondition {
        fromJson<QString>(jo["room_id"_L1]),
        fromJson<QString>(jo["type"_L1])
    };
}

template<>
inline auto toJson(const EventContent::AllowCondition& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, "room_id"_L1, c.roomId);
    addParam<IfNotEmpty>(jo, "type"_L1, c.type);
    return jo;
}

template<>
inline EventContent::JoinRuleContent fromJson(const QJsonObject& jo)
{
    return EventContent::JoinRuleContent {
        enumFromJsonString<EventContent::JoinRule>(jo["join_rule"_L1].toString(), EventContent::JoinRuleStrings).value_or(EventContent::Public),
        fromJson<QList<EventContent::AllowCondition>>(jo["allow"_L1])
    };
}

template<>
inline auto toJson(const EventContent::JoinRuleContent& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, "join_rule"_L1, enumToJsonString<EventContent::JoinRule>(c.joinRule, EventContent::JoinRuleStrings));
    addParam<IfNotEmpty>(jo, "allow"_L1, c.allow);
    return jo;
}

//! \brief Class to define a join rule state event.
//!
//! \sa Quotient::StateEvent, https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
class JoinRulesEvent : public KeylessStateEventBase<JoinRulesEvent,
    EventContent::JoinRuleContent>
{
public:
    QUO_EVENT(JoinRulesEvent, "m.room.join_rules")
    using KeylessStateEventBase::KeylessStateEventBase;

    //! \brief The join rule for the room.
    //!
    //! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
    EventContent::JoinRule joinRule() const { return content().joinRule; }

    //! \brief The allow rules for restricted rooms.
    //!
    //! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
    QList<EventContent::AllowCondition> allow() const { return content().allow; }
};
} // namespace Quotient
