// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomstateview.h"

using namespace Quotient;

const StateEvent* RoomStateView::get(const QString& evtType,
                                     const QString& stateKey) const
{
    return value({ evtType, stateKey });
}

bool RoomStateView::contains(const QString& evtType,
                             const QString& stateKey) const
{
    return contains({ evtType, stateKey });
}

QJsonObject RoomStateView::contentJson(const QString& evtType,
                                       const QString& stateKey) const
{
    return queryOr(evtType, stateKey, &Event::contentJson, QJsonObject());
}

QVector<const StateEvent*> RoomStateView::eventsOfType(const QString& evtType) const
{
    using namespace std::ranges;
    const auto& kvRange = asKeyValueRange();
    return rangeTo<QVector>(views::filter(kvRange, [evtType](const auto& kv) {
        return kv.first.first == evtType;
    }) | views::values);
}
