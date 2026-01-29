// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "thread.h"

#include "room.h"

using namespace Quotient;

ThreadInfos::UpdateResult ThreadInfos::updateFrom(const TimelineItem &eventItem)
{
    if (QUO_ALARM_X(!room, "Initialize ThreadInfos before calling updateThread()"))
        return {};

    const auto &event = *eventItem;
    const auto &threadRootId = event.threadRootEventId();
    if (threadRootId.isEmpty())
        return {};

    const auto isRootEvent = contains(event.id())
                             || event.hasRelationship(EventRelation::ThreadType);
    // Check whether the event is (already known to be) a thread root
    if (isRootEvent) {
        if (const auto relation = event.relatesTo();
            relation && relation->type == EventRelation::ThreadType)
            return {}; // Malformed event (looks like both root and leaf), skip
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    auto [threadIt, isNew] = tryEmplace(threadRootId, event.id());
#else
    auto threadIt = base_type::find(threadRootId); // base_type:: to use non-const find()
    const auto isNew = threadIt == end();
    if (isNew)
        threadIt = insert(threadRootId, {event.id()});
#endif

    const auto updateInfo = [this, threadIt](const RoomEvent &evt) {
        ++threadIt->size;
        threadIt->localUserParticipated |= (evt.senderId() == room->connection()->userId());
        return *threadIt;
    };

    // The difference of adding a root event from adding a leaf event is:
    // 1. In case of root events latestEventId is determined by the emplacement result: if we've
    //    just inserted a new thread info then the latest event is the only one just added, by
    //    construction; and if a thread info structure is already there then some leaf event in it
    //    has been processed before, so the root event cannot be the latest, also by construction.
    // 2. We don't need to look for a prior event that we now know is the thread root.
    if (!isRootEvent)
    {
        if (isNew) {
            // Look around for the thread root event and update from it, too
            if (auto rootIt = room->findInTimeline(threadRootId); rootIt != room->historyEdge())
                updateInfo(**rootIt);
            // If we can't find the root assume it's a historical event and will be loaded later
        } else {
            const auto threadLatestIt = room->findInTimeline(threadIt->latestEventId);
            if (threadLatestIt == room->historyEdge() || eventItem.index() > threadLatestIt->index())
                threadIt->latestEventId = event.id();
        }
    }

    return { updateInfo(event), isNew };
}
