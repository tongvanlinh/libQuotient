// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "memberproxy.h"

#include "logging_categories_p.h"
#include "room.h"

namespace Quotient {

MemberListProxy::MemberListProxy(const Room *room, QList<const RoomMemberEvent *> memberEvents)
    : _room(room), _memberEvents(std::move(memberEvents))
{
    fillMemberIds();
    if (_room)
        _roomListener = QObject::connect(_room, &Room::memberListChanged, [this] {
            qWarning(EVENTS)
                << "Refilling MemberListProxy from ids to avoid dangling event pointers";
            qWarning(EVENTS)
                << "Do not store MemberListProxy objects, they are not meant to be long-lived";
            auto newEventsRng = _room->currentState().someEventsOfType<RoomMemberEvent>(_memberIds);
            _memberEvents.assign(newEventsRng.begin(), newEventsRng.end());
            fillMemberIds(); // Some members might have left
        });
}

MemberListProxy::~MemberListProxy() { QObject::disconnect(_roomListener); }

MemberProxy MemberListProxy::firstNonLocal() const
{
    for (auto it = _memberIds.cbegin(); it != _memberIds.cend(); ++it)
        if (*it != _room->localUserId())
            return MemberProxy(_room, _memberEvents[it - _memberIds.cbegin()]);
    return {};
}

MemberListProxy::iterator &MemberListProxy::iterator::operator++()
{
    ++_it;
    return *this;
}

MemberListProxy::iterator MemberListProxy::iterator::operator++(int)
{
    iterator copy = *this;
    ++*this;
    return copy;
}

MemberListProxy::iterator &MemberListProxy::iterator::operator--()
{
    --_it;
    return *this;
}

MemberListProxy::iterator MemberListProxy::iterator::operator--(int)
{
    iterator copy = *this;
    --*this;
    return copy;
}

MemberListProxy::iterator &MemberListProxy::iterator::operator+=(difference_type n)
{
    _it += n;
    return *this;
}

MemberListProxy::iterator &MemberListProxy::iterator::operator-=(difference_type n)
{
    _it -= n;
    return *this;
}

std::partial_ordering MemberListProxy::iterator::compareWith(iterator rhs) const
{
    if (_room != rhs._room)
        return std::partial_ordering::unordered;

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    return _it <=> rhs._it;
#else
    return _it < rhs._it    ? std::partial_ordering::less
           : _it == rhs._it ? std::partial_ordering::equivalent
                            : std::partial_ordering::greater;
#endif
}

void MemberListProxy::fillMemberIds()
{
    _memberIds.resize(_memberEvents.size());
    std::ranges::transform(_memberEvents, _memberIds.begin(), &StateEvent::stateKey);
}

} // namespace Quotient
