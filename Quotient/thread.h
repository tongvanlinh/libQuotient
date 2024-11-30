// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QString>

#include "quotient_export.h"

namespace Quotient {

class RoomMessageEvent;

class QUOTIENT_API Thread {
public:
    QString threadRootId = {};
    QString latestEventId = {};
    int size = 0;
    bool localUserParticipated = {};

    bool addEvent(const RoomMessageEvent* event, bool isLatest, bool isLocalUser);
};

} // namespace Quotient
