// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QString>
#include <QJsonObject>

#include "quotient_export.h"

namespace Quotient {

class RoomMessageEvent;

class QUOTIENT_API Thread {
public:
    explicit Thread(QString threadRootId, QString latestEventId, int size, bool localUserParticipated);

    QString threadRootId() const;
    QString latestEventId() const;
    int size() const;
    bool localUserParticipated() const;

    bool addEvent(const RoomMessageEvent* event, bool isLatest, bool isLocalUser);

private:
    const QString _threadRootId;
    QString _latestEventId;
    int _size;
    bool _localUserParticipated;
};

class QUOTIENT_API ThreadView {
public:
    ThreadView() = default;

    void add(std::unique_ptr<Thread> thread);

    bool erase(const QString& threadRootId);

    bool exisits(const QString& threadRootId) const;

    Thread* getThread(const QString& threadRootId) const;

private:
    std::vector<std::unique_ptr<Thread>> _threads;
};

} // namespace Quotient
