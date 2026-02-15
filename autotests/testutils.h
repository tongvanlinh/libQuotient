// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <memory>

#include <Quotient/events/event.h>

namespace Quotient {

template<EventClass EventT>
inline event_ptr_tt<EventT> loadEventFromFile(const QString &eventFileName)
{
    if (!eventFileName.isEmpty()) {
        if (QFile testEventFile(QStringLiteral(DATA_DIR "/") + eventFileName);
            testEventFile.open(QIODevice::ReadOnly)) {
            return loadEvent<EventT>(QJsonDocument::fromJson(testEventFile.readAll()).object());
        }
    }
    return nullptr;
}

class Connection;
class Room;
template <class JobT>
class JobHandle;

std::shared_ptr<Connection> createTestConnection(QLatin1StringView localUserName,
                                                 QLatin1StringView secret,
                                                 QLatin1StringView deviceName);

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME)             \
    const auto VAR = createTestConnection(USERNAME, SECRET, DEVICE_NAME); \
    QVERIFY2(VAR != nullptr, "Could not set up test connection")

std::pair<Room *, Room *> createTestChat(Connection *c1, Connection *c2);

constexpr inline auto DefaultWaitTimeout = std::chrono::seconds(60);

inline bool waitForSignal(auto objPtr, auto signal)
{
    return QSignalSpy(std::to_address(objPtr), signal).wait(DefaultWaitTimeout);
}

template <typename T>
inline bool waitForFuture(const QFuture<T> &ft,
                          QDeadlineTimer timeout = QDeadlineTimer(DefaultWaitTimeout))
{
    return QTest::qWaitFor([ft] { return ft.isFinished(); }, timeout);
}

template <typename JobT>
inline bool waitForJob(const JobHandle<JobT> &job)
{
    using namespace std::chrono_literals;
    const auto &[timeouts, retryIntervals, _] = job->currentBackoffStrategy();
    const auto timeout =
#ifdef __cpp_lib_ranges_fold // libc 20 and Apple Clang 16 still don't have it
        std::ranges::fold_left(timeouts, 0ms, std::plus{})
        + std::ranges::fold_left(retryIntervals, 0ms, std::plus{});
#else
        std::chrono::milliseconds(std::reduce(timeouts.cbegin(), timeouts.cend())
                                  + std::reduce(retryIntervals.cbegin(), retryIntervals.cend()));
#endif
    return waitForFuture(job, timeout);
}

}
