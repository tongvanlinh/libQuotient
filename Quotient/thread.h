// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-FileCopyrightText: 2026 Alexey Rusakov <kitsune@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventitem.h"

#include <QtCore/QHash>

namespace Quotient {
class Room;

//! \brief Simple data structure representing a thread
//!
//! ThreadInfo objects are managed by ThreadInfos, which stores them keyed by thread root event ids.
//! The thread root id is not stored in ThreadInfo itself to avoid redundancy.
//! \sa ThreadInfos
struct QUOTIENT_API ThreadInfo {
    QString latestEventId;
    int size = 0;
    bool localUserParticipated = {};
};

//! \brief A manager of thread information in a room
//!
//! ThreadInfos encapsulates thread management logic, storing threads keyed by their root event id.
//! Every time an event arrives to the timeline the room should send it to updateFrom() to update
//! the relevant thread info if there's any thread involved. Otherwise this class behaves as
//! a read-only `QHash<QString, ThreadInfo>`
//! \sa ThreadInfo
class QUOTIENT_API ThreadInfos : private QHash<QString, ThreadInfo> {
public:
    using base_type = QHash<QString, ThreadInfo>;
    using const_iterator = base_type::const_iterator;
    using key_type = base_type::key_type;
    using mapped_type = base_type::mapped_type;

    using UpdateResult = std::pair<ThreadInfo, bool>;

    //! \brief Update thread state for an event that just arrived to the timeline
    //!
    //! This method handles the full logic of updating thread state when a new event arrives:
    //! it figures out if any thread should be created or updated, and then creates or updates
    //! the respective ThreadInfo object according to the data in the event.
    //! \note As of this writing, the event reference itself is not stored, only the aggregated
    //!       statistics.
    //! \return A pair consisting of a ThreadInfo object that was created/updated, and a boolean
    //!         flag that is `true` if that object was created, `false` if it was updated
    UpdateResult updateFrom(const TimelineItem &eventItem);

    // Exposing the relevant read-only API of QHash

    base_type::const_iterator begin() const { return base_type::begin(); }
    base_type::const_iterator end() const { return base_type::end(); }
    base_type::const_iterator find(const key_type &k) const { return base_type::find(k); }
    mapped_type operator[](const key_type &k) const { return base_type::operator[](k);  }

    using base_type::cbegin, base_type::cend, base_type::constFind, base_type::value;
    using base_type::contains, base_type::size, base_type::isEmpty;

private:
    Room* room = nullptr;

    friend class Room;
    void setRoom(Room* r) { room = r; }
};

} // namespace Quotient
