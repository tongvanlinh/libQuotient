// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testthread.h"
#include <Quotient/events/roommessageevent.h>
#include <Quotient/thread.h>

#include "testutils.h"

using namespace Quotient;

void TestThread::newThread()
{
    auto testThread = Thread();
    const auto rootEvent = loadEventFromFile<RoomMessageEvent>("test-threadroot-event.json"_L1);

    testThread.threadRootId = rootEvent->id();
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, QString());
    QCOMPARE(testThread.size, 0);
    QCOMPARE(testThread.localUserParticipated, false);

    testThread.addEvent(rootEvent.get(), true, false);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.size, 1);
    QCOMPARE(testThread.localUserParticipated, false);

    const auto replyEvent1 = loadEventFromFile<RoomMessageEvent>("test-thread1-event.json"_L1);
    testThread.addEvent(replyEvent1.get(), true, true);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$thread1:example.org"_L1);
    QCOMPARE(testThread.size, 2);
    QCOMPARE(testThread.localUserParticipated, true);

    const auto replyEvent2 = loadEventFromFile<RoomMessageEvent>("test-thread2-event.json"_L1);
    testThread.addEvent(replyEvent2.get(), true, false);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$thread2:example.org"_L1);
    QCOMPARE(testThread.size, 3);
    QCOMPARE(testThread.localUserParticipated, true);
}

void TestThread::historicalThread()
{
    auto testThread = Thread();
    const auto replyEvent2 = loadEventFromFile<RoomMessageEvent>("test-thread2-event.json"_L1);

    testThread.threadRootId = replyEvent2->threadRootEventId();
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, QString());
    QCOMPARE(testThread.size, 0);
    QCOMPARE(testThread.localUserParticipated, false);

    testThread.addEvent(replyEvent2.get(), true, false);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$thread2:example.org"_L1);
    QCOMPARE(testThread.size, 1);
    QCOMPARE(testThread.localUserParticipated, false);

    const auto replyEvent1 = loadEventFromFile<RoomMessageEvent>("test-thread1-event.json"_L1);
    testThread.addEvent(replyEvent1.get(), false, true);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$thread2:example.org"_L1);
    QCOMPARE(testThread.size, 2);
    QCOMPARE(testThread.localUserParticipated, true);

    const auto rootEvent = loadEventFromFile<RoomMessageEvent>("test-threadroot-event.json"_L1);
    testThread.addEvent(rootEvent.get(), false, true);
    QCOMPARE(testThread.threadRootId, "$threadroot:example.org"_L1);
    QCOMPARE(testThread.latestEventId, "$thread2:example.org"_L1);
    QCOMPARE(testThread.size, 3);
    QCOMPARE(testThread.localUserParticipated, true);
}

QTEST_GUILESS_MAIN(TestThread)
