// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtTest/QTest>

#include <memory>

#include <Quotient/events/event.h>

namespace Quotient {

class Connection;
template <class JobT>
class JobHandle;

std::shared_ptr<Connection> createTestConnection(QLatin1StringView localUserName,
                                                 QLatin1StringView secret,
                                                 QLatin1StringView deviceName);


template<EventClass EventT>
inline event_ptr_tt<EventT> loadEventFromFile(const QString &eventFileName)
{
    if (!eventFileName.isEmpty()) {
        QFile testEventFile;
        testEventFile.setFileName(QLatin1String(DATA_DIR) + u'/' + eventFileName);
        testEventFile.open(QIODevice::ReadOnly);
        return loadEvent<EventT>(QJsonDocument::fromJson(testEventFile.readAll()).object());
    }
    return nullptr;
}
}

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME)             \
    const auto VAR = createTestConnection(USERNAME, SECRET, DEVICE_NAME); \
    if (!VAR)                                                             \
        QFAIL("Could not set up test connection");

template <typename JobT>
inline bool waitForJob(const Quotient::JobHandle<JobT>& job)
{
    const auto& [timeouts, retryIntervals, _] = job->currentBackoffStrategy();
    return QTest::qWaitFor([job] { return job.isFinished(); },
                           std::chrono::milliseconds(
                               std::reduce(timeouts.cbegin(), timeouts.cend())
                               + std::reduce(retryIntervals.cbegin(), retryIntervals.cend()))
                               .count());
}
