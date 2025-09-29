// SPDX-FileCopyrightText: 2025 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../quotient_common.h"
#include "stateevent.h"

namespace Quotient
{
namespace EventContent {

//! \brief Definition of an allow AllowCondition
//!
//! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_ruless
struct QUOTIENT_API AllowCondition {
    QString roomId;
    QString type;
};

//! \brief The content of a join rule event
//!
//! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
struct QUOTIENT_API JoinRuleContent {
    JoinRule joinRule;
    QList<AllowCondition> allow;
};
} // namespace EventContent

constexpr inline auto JoinRuleKey = "join_rule"_L1;
constexpr inline auto AllowKey = "allow"_L1;

template<>
inline EventContent::AllowCondition fromJson(const QJsonObject& jo)
{
    return EventContent::AllowCondition {
        fromJson<QString>(jo[RoomIdKey]),
        fromJson<QString>(jo[TypeKey])
    };
}

template<>
inline auto toJson(const EventContent::AllowCondition& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, RoomIdKey, c.roomId);
    addParam<IfNotEmpty>(jo, TypeKey, c.type);
    return jo;
}

template<>
inline EventContent::JoinRuleContent fromJson(const QJsonObject& jo)
{
    return EventContent::JoinRuleContent{fromJson<JoinRule>(jo[JoinRuleKey]),
                                         fromJson<QList<EventContent::AllowCondition>>(
                                             jo[AllowKey])};
}

template<>
inline auto toJson(const EventContent::JoinRuleContent& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, JoinRuleKey, c.joinRule);
    addParam<IfNotEmpty>(jo, AllowKey, c.allow);
    return jo;
}

//! \brief Class to define a join rule state event.
//!
//! \sa Quotient::StateEvent, https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
class QUOTIENT_API JoinRulesEvent
    : public KeylessStateEventBase<JoinRulesEvent, EventContent::JoinRuleContent>
{
public:
    QUO_EVENT(JoinRulesEvent, "m.room.join_rules")
    using KeylessStateEventBase::KeylessStateEventBase;

    //! \brief The join rule for the room.
    //!
    //! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
    JoinRule joinRule() const { return content().joinRule; }

    //! \brief The allow rules for restricted rooms.
    //!
    //! \sa https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules
    QList<EventContent::AllowCondition> allow() const { return content().allow; }
};
} // namespace Quotient
