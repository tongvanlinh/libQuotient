// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "threadview.h"

#include "events/roommessageevent.h"

using namespace Quotient;

void ThreadView::add(std::unique_ptr<Thread> thread)
{
    _threads.push_back(std::move(thread));
}

bool ThreadView::erase(const QString& threadRootId)
{
    auto threadIt = std::find_if(_threads.begin(), _threads.end(), [threadRootId](const auto& thread) { return thread->threadRootId() == threadRootId; });
    if (threadIt == _threads.end()) {
        return false;
    }
     _threads.erase(threadIt);
    return true;
}

bool ThreadView::exisits(const QString& threadRootId) const
{
    return std::find_if(_threads.begin(), _threads.end(), [threadRootId](const auto& thread) { return thread->threadRootId() == threadRootId; }) != _threads.end();
}

Thread* ThreadView::getThread(const QString& threadRootId) const
{
    auto threadIt = std::find_if(_threads.begin(), _threads.end(), [threadRootId](const auto& thread) { return thread->threadRootId() == threadRootId; });
    if (threadIt == _threads.end()) {
        return nullptr;
    }
    return threadIt->get();
}

Thread::Thread(QString threadRootId, QString latestEventId, int size, bool localUserParticipated)
    : _threadRootId(threadRootId), _latestEventId(threadRootId), _size(size), _localUserParticipated(localUserParticipated)
{}

QString Thread::threadRootId() const
{
    return _threadRootId;
}

QString Thread::latestEventId() const
{
    return _latestEventId;
}

int Thread::size() const
{
    return _size;
}

bool Thread::localUserParticipated() const
{
    return _localUserParticipated;
}

bool Thread::addEvent(const RoomMessageEvent* event, bool isLatest, bool isLocalUser)
{
    if (event->threadRootEventId() != _threadRootId) {
        return false;
    }
    if (isLatest) {
        _latestEventId = event->id();
    }
    _size++;
    if (!_localUserParticipated) {
        _localUserParticipated = isLocalUser;
    }

    return true;
}
