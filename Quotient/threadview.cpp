// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "threadview.h"

#include "events/roommessageevent.h"

using namespace Quotient;

namespace {
inline auto checkThreadRoot(const QString& threadRootId) {
    return [threadRootId](const std::unique_ptr<Thread>& thread) { return threadRootId == thread->threadRootId(); };
}
}

void ThreadView::add(std::unique_ptr<Thread> thread)
{
    _threads.push_back(std::move(thread));
}

bool ThreadView::erase(const QString& threadRootId)
{
    return std::erase_if(_threads, checkThreadRoot(threadRootId)) > 0;
}

bool ThreadView::exists(const QString& threadRootId) const
{
    return std::ranges::any_of(_threads, checkThreadRoot(threadRootId));
}

Thread* ThreadView::getThread(const QString& threadRootId) const
{
    auto threadIt = std::ranges::find_if(_threads, checkThreadRoot(threadRootId));
    if (threadIt == _threads.end()) {
        return nullptr;
    }
    return threadIt->get();
}

Thread::Thread(QString threadRootId, QString latestEventId, int size, bool localUserParticipated)
    : _threadRootId(threadRootId), _latestEventId(latestEventId), _size(size), _localUserParticipated(localUserParticipated)
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
    ++_size;
    if (!_localUserParticipated) {
        _localUserParticipated = isLocalUser;
    }

    return true;
}
