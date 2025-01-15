// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "thread.h"

#include "events/roommessageevent.h"

using namespace Quotient;

bool Thread::addEvent(const RoomMessageEvent* event, bool isLatest, bool isLocalUser)
{
    // Note: the root event may not have the thread aggregation in its unsigned on creation
    // hence checking the event id.
    if (event->threadRootEventId() != threadRootId && event->id() != threadRootId) {
        return false;
    }
    if (isLatest || latestEventId.isEmpty()) {
        latestEventId = event->id();
    }
    ++size;
    localUserParticipated |= isLocalUser;

    return true;
}
