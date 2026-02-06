// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testutils.h"
#include <Quotient/csapi/inviting.h>
#include <Quotient/events/roommessageevent.h>
#include <Quotient/events/stickerevent.h>
#include <Quotient/room.h>
#include <Quotient/thread.h>

#include <QtTest/QTest>

using namespace Quotient;

namespace {

//! Wait until the pending event item is merged into the timeline
QString waitForEventMerged(const PendingEventItem &item)
{
    auto ft = item.whenMerged();
    return waitForFuture(ft) ? ft.result().get().id() : QString();
}

//! \brief Wait for another user's event(s) to appear in the timeline
//!
//! This is for the case when the user whom the \p room object belongs to expects events from some
//! other user (cf. waitForEventMerged() where the user waits for the event they sent themselves).
bool waitForEventsInTimeline(Room *room, const auto &...eventIds)
{
    if (!room)
        qFatal("waitForEventsInTimeline(): room is null");
    if ((eventIds.isEmpty() || ...))
        qFatal("waitForEventsInTimeline(): at least one of eventIds is empty");

    return QTest::qWaitFor([room, &eventIds...] {
        return ((room->findInTimeline(eventIds) != room->historyEdge()) && ...);
    }, DefaultWaitTimeout);
}

//! Common event exchange for all three test cases
std::array<QString, 3> exchangeThreadEvents(Room *aliceRoom, Room *bobRoom)
{
    if (aliceRoom->id() != bobRoom->id())
        qFatal("Room views of the two members don't match");

    // alice sends root message
    auto rootEventId = waitForEventMerged(aliceRoom->post<RoomMessageEvent>("Root message"_L1));
    if (rootEventId.isEmpty())
        return {};

    // bob replies in thread
    const auto replyEventId = waitForEventMerged(bobRoom->post<RoomMessageEvent>(
        "Thread reply from bob"_L1, RoomMessageEvent::MsgType::Text, nullptr,
        EventRelation::replyInThread(rootEventId, false, rootEventId)));
    if (replyEventId.isEmpty())
        return {rootEventId};

    // alice replies with a sticker
    auto stickerEvent = makeEvent<StickerEvent>(
        u"Sticker"_s, EventContent::ImageContent(QUrl(u"mxc://localhost:1234/sticker123"_s)));
    const auto stickerRelation = EventRelation::replyInThread(rootEventId, false, rootEventId);
    stickerEvent->setRelation(stickerRelation);
    const auto stickerEventId = waitForEventMerged(aliceRoom->post(std::move(stickerEvent)));
    if (!stickerEventId.isEmpty())
        waitForEventsInTimeline(bobRoom, rootEventId, stickerEventId);

    return {rootEventId, replyEventId, stickerEventId};
}

#define CHECK_THREAD(RootEventId_, ReplyEventId_, StickerEventId_)                                \
    do {                                                                                          \
        QVERIFY2(!RootEventId_.isEmpty(), "Thread root event is not good, see the log above");    \
        QVERIFY2(!ReplyEventId_.isEmpty(), " First thread reply is not good, see the log above"); \
        QVERIFY2(!StickerEventId_.isEmpty(), "Sticker reply is not good, see the log above");     \
    } while (false)

} // anonymous namespace

class TestThread : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void newThread();
    void historicalThread();
    void rootAlreadyThreaded();
    void cleanup();

private:
    std::shared_ptr<Connection> alice = nullptr, bob = nullptr, carl = nullptr;
    Room *aliceRoom = nullptr, *bobRoom = nullptr;
};

void TestThread::initTestCase()
{
    // Not meaning to test E2EE here, and it slows down the test considerably
    Connection::setEncryptionDefault(false);

    // Set up connections common for all tests
    alice = createTestConnection("alice1"_L1, "secret"_L1, "AliceDevice"_L1);
    QVERIFY(alice != nullptr);
    bob = createTestConnection("bob1"_L1, "secret"_L1, "BobDevice"_L1);
    QVERIFY(bob != nullptr);
    carl = createTestConnection("carl"_L1, "secret"_L1, "CarlDevice"_L1);
    QVERIFY(carl != nullptr);
    alice->syncLoop();
    bob->syncLoop();
    // carl doesn't need a sync loop; where necessary, individual sync()s will be called
}

void TestThread::init()
{
    std::tie(aliceRoom, bobRoom) = createTestChat(alice.get(), bob.get());
    QVERIFY2(aliceRoom != nullptr, "Failed to create a test room for Alice");
    QVERIFY2(bobRoom != nullptr, "Failed to join the test room for Bob");
    QVERIFY(aliceRoom->id() == bobRoom->id());
}

void TestThread::cleanup()
{
    for (Room **r : {&aliceRoom, &bobRoom}) {
        (*r)->leaveRoom();
        (*r) = nullptr;
    }
}

void TestThread::newThread()
{
    const auto [rootEventId, replyEventId, stickerEventId] =
        exchangeThreadEvents(aliceRoom, bobRoom);
    CHECK_THREAD(rootEventId, replyEventId, stickerEventId);

    // Verify events are visible on both sides
    QVERIFY(bobRoom->findInTimeline(rootEventId) != bobRoom->historyEdge());
    QVERIFY(aliceRoom->findInTimeline(replyEventId) != aliceRoom->historyEdge());
    QVERIFY(bobRoom->findInTimeline(stickerEventId) != bobRoom->historyEdge());

    // Verify thread info on both sides
    for (const auto& threads : { aliceRoom->threads(), bobRoom->threads() }) {
        QVERIFY(threads.contains(rootEventId));
        const auto t = threads[rootEventId];
        QCOMPARE(t.latestEventId, stickerEventId);
        QCOMPARE(t.size, 3);
        QVERIFY(t.localUserParticipated);
    }
}

void TestThread::historicalThread()
{
    const auto [rootEventId, replyEventId, stickerEventId] =
        exchangeThreadEvents(aliceRoom, bobRoom);
    CHECK_THREAD(rootEventId, replyEventId, stickerEventId);

    const auto roomId = aliceRoom->id();

    // Invite and join carl after the events exchange - he doesn't even need to sync, only to load
    // history, to check the thread
    auto carlRoomFuture = alice->callApi<InviteUserJob>(roomId, carl->userId())
                              .then([this, roomId] { return carl->joinAndGetRoom(roomId); })
                              .unwrap()
                              .then([rootEventId, replyEventId, stickerEventId](Room *carlRoom) {
        auto carlGuard = qScopeGuard([carlRoom] { carlRoom->leaveRoom(); });
        auto historyJob = carlRoom->getPreviousContent();
        QVERIFY(waitForJob(historyJob));

        // Verify all three events are in the timeline
        QVERIFY2(carlRoom->findInTimeline(rootEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see thread root event");
        QVERIFY2(carlRoom->findInTimeline(replyEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see the first reply in the thread");
        QVERIFY2(carlRoom->findInTimeline(stickerEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see the sticker reply");

        // Verify thread info
        const auto& threads = carlRoom->threads();
        QVERIFY(threads.contains(rootEventId));
        const auto t = threads[rootEventId];
        QCOMPARE(t.latestEventId, stickerEventId);
        QCOMPARE(t.size, 3);
        QCOMPARE(t.localUserParticipated, false);
    });
    waitForFuture(carlRoomFuture, DefaultWaitTimeout * 2);
}

void TestThread::rootAlreadyThreaded()
{
    const auto roomId = aliceRoom->id();

    // Invite and join carl before events exchange
    auto carlRoomFuture = alice->callApi<InviteUserJob>(roomId, carl->userId())
                              .then([this, roomId] { return carl->joinAndGetRoom(roomId); })
                              .unwrap()
                              .then([this](Room *carlRoom) {
        auto carlGuard = qScopeGuard([carlRoom] { carlRoom->leaveRoom(); });

        // carl does a single sync before events exchange
        carl->sync();
        QVERIFY(waitForSignal(carl, &Connection::syncDone));

        const auto [rootEventId, replyEventId, stickerEventId] =
            exchangeThreadEvents(aliceRoom, bobRoom);
        CHECK_THREAD(rootEventId, replyEventId, stickerEventId);

        // carl now runs a sync loop to get all three events (one sync may not be enough)
        carl->syncLoop();
        QVERIFY(waitForEventsInTimeline(carlRoom, rootEventId, replyEventId, stickerEventId));

        // Verify all three events are in carl's timeline
        QVERIFY2(carlRoom->findInTimeline(rootEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see thread root event");
        QVERIFY2(carlRoom->findInTimeline(replyEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see the first reply in the thread");
        QVERIFY2(carlRoom->findInTimeline(stickerEventId) != carlRoom->historyEdge(),
                 "Carl doesn't see the sticker reply");

        // Verify the root event is marked as threaded
        const auto carlRootIt = carlRoom->findInTimeline(rootEventId);
        QVERIFY(carlRootIt != carlRoom->historyEdge());
        // TODO: figure out why local Synapse doesn't aggregate the thread
        // QVERIFY((*carlRootIt)->hasRelationship(EventRelation::ThreadType));
        // QVERIFY((*carlRootIt)->isThreaded());

        // Verify thread info
        const auto& threads = carlRoom->threads();
        QVERIFY(threads.contains(rootEventId));
        const auto t = threads[rootEventId];
        QCOMPARE(t.latestEventId, stickerEventId);
        QCOMPARE(t.size, 3);
        QCOMPARE(t.localUserParticipated, false);
    });
    waitForFuture(carlRoomFuture, DefaultWaitTimeout * 2);
}

QTEST_GUILESS_MAIN(TestThread)
#include "testthread.moc"
