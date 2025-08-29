// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

#include "eventrelation.h"

#include <QtCore/QDateTime>

namespace Quotient {

constexpr inline auto EventIdKey = "event_id"_L1;
constexpr inline auto RoomIdKey = "room_id"_L1;
constexpr inline auto StateKeyKey = "state_key"_L1;
constexpr inline auto RedactedCauseKey = "redacted_because"_L1;

class RedactionEvent;
class EncryptedEvent;

// That check could look into Event and find most stuff already deleted...
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class QUOTIENT_API RoomEvent : public Event {
public:
    QUO_BASE_EVENT(RoomEvent, Event)

    ~RoomEvent() override; // Don't inline this - see the private section

    //! \brief A convenience function to get a display string for an event ID.
    //! \return id() if the event id is not empty, otherwise transactionId();
    //!              this is useful to deal with pending and normal events uniformly.
    //! \sa id(), transactionId()
    QString displayId() const;

    //! The event_id JSON value for the event.
    QString id() const;

    QDateTime originTimestamp() const;
    QString roomId() const;
    QString senderId() const;
    bool isRedacted() const { return bool(_redactedBecause); }
    const event_ptr_tt<RedactionEvent>& redactedBecause() const
    {
        return _redactedBecause;
    }
    QString redactionReason() const;

    //! The transaction_id JSON value for the event.
    QString transactionId() const;

    // State events are special in Matrix; so isStateEvent() and stateKey() are here,
    // as an exception. For other event types (including base types), Event::is<>() and
    // Quotient::is<>() should be used

    bool isStateEvent() const;

    QString stateKey() const;

    //! \brief Fill the pending event object with the room id
    void setRoomId(const QString& roomId);
    //! \brief Fill the pending event object with the sender id
    void setSender(const QString& senderId);
    //! \brief Fill the pending event object with the transaction id
    //! \param txnId - transaction id, normally obtained from
    //!        Connection::generateTxnId()
    void setTransactionId(const QString& txnId);

    //! \brief Add an event id to locally created events after they are sent
    //!
    //! When a new event is created locally, it has no id; the homeserver
    //! assigns it once the event is sent. This function allows to add the id
    //! once the confirmation from the server is received. There should be no id
    //! set previously in the event. It's the responsibility of the code calling
    //! addId() to notify clients about the change; there's no signal or
    //! callback for that in RoomEvent.
    void addId(const QString& newId);

    void setOriginalEvent(event_ptr_tt<EncryptedEvent>&& originalEvent);
    const EncryptedEvent* originalEvent() const { return _originalEvent.get(); }
    const QJsonObject encryptedJson() const;

    //! \brief Determine whether the event is a reply to another message.
    //!
    //! \param includeFallbacks include thread fallback replies for non-threaded clients.
    //!
    //! \return true if this event is a reply, i.e. it has `"m.in_reply_to"`
    //!         event ID and is not a thread fallback (except where \p includeFallbacks is true);
    //!         false otherwise.
    //!
    //! \note It's possible to reply to another message in a thread so this function
    //!       will return true for a `"rel_type"` of `"m.thread"` if `"is_falling_back"`
    //!       is false.
    bool isReply(bool includeFallbacks = false) const;

    //! \brief The ID for the event being replied to.
    //!
    //! \param includeFallbacks include thread fallback replies for non-threaded clients.
    //!
    //!
    //! \return The event ID for a reply, this includes threaded replies where `"rel_type"`
    //!         is `"m.thread"` and `"is_falling_back"` is false (except where \p includeFallbacks is true).
    QString replyEventId(bool includeFallbacks = false) const;

    //! \brief The EventRelation for this event.
    //!
    //! \return an EventRelation object which can be checked for type if it exists,
    //!         std::nullopt otherwise.
    std::optional<EventRelation> relatesTo() const;

protected:
    explicit RoomEvent(const QJsonObject& json);
    void dumpTo(QDebug dbg) const override;

private:
    // RedactionEvent is an incomplete type here so we cannot inline
    // constructors using it and also destructors (with 'using', in particular).
    event_ptr_tt<RedactionEvent> _redactedBecause;

    event_ptr_tt<EncryptedEvent> _originalEvent;
};
using RoomEventPtr = event_ptr_tt<RoomEvent>;
using RoomEvents = EventsArray<RoomEvent>;
using RoomEventsRange = std::ranges::subrange<RoomEvents::iterator>;

//! \brief Determine whether a given event type is that of a state event
QUOTIENT_API bool isStateEvent(const QString& eventTypeId);

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::RoomEvent*)
Q_DECLARE_METATYPE(const Quotient::RoomEvent*)
