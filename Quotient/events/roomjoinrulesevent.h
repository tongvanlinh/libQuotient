// SPDX-FileCopyrightText: 2025 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/events/stateevent.h>

namespace Quotient
{
namespace EventContent {
    //! \brief Definition of an allow AllowCondition
    //!
    //! see https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules for the
    //! full definition.
    struct AllowCondition {
        QString roomId;
        QString type;
    };

    //! \brief The content of a join rule event
    //!
    //! see https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules for the
    //! full definition.
    struct JoinRuleContent {
        QString joinRule;
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
        fromJson<QString>(jo["join_rule"_L1]),
        fromJson<QList<EventContent::AllowCondition>>(jo["allow"_L1])
    };
}

template<>
inline auto toJson(const EventContent::JoinRuleContent& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, "join_rule"_L1, c.joinRule);
    addParam<IfNotEmpty>(jo, "allow"_L1, c.allow);
    return jo;
}

//! \brief Class to define a join rule state event.
//!
//! see https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules for the
//! full definition.
//!
//! \sa Quotient::StateEvent
class JoinRulesEvent : public KeylessStateEventBase<JoinRulesEvent,
    EventContent::JoinRuleContent>
{
public:
    QUO_EVENT(JoinRulesEvent, "m.room.join_rules")
    using KeylessStateEventBase::KeylessStateEventBase;

    //! \brief The join rule for the room.
    //!
    //! see https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules for
    //! the available join rules for a room.
    QString joinRule() const { return content().joinRule; }

    //! \brief The allow rules for restricted rooms.
    //!
    //! see https://spec.matrix.org/latest/client-server-api/#mroomjoin_rules for
    //! the full details on allow rules.
    QList<EventContent::AllowCondition> allow() const { return content().allow; }
};
} // namespace Quotient
