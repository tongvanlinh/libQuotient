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

    //! \brief Make a redacted event
    //!
    //! This applies the redaction procedure as defined by the CS API specification to the event's
    //! JSON and returns the resulting new event.
    //! \note It is the responsibility of the caller to dispose of the original event after that.
    event_ptr_tt<RoomEvent> makeRedacted(const RedactionEvent &redaction) const;

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

    //! \brief Set the event relation data
    //!
    //! Adds the relation to another event with the contents of \p er. If another relation
    //! exists it is entirely overwritten.
    void setRelation(const EventRelation &er);

    //! \brief Remove the event relation data
    //!
    //! Clears any relation from this event to another event.
    //! \sa setRelation
    void clearRelation();

    //! \brief Get relations to this event
    //!
    //! This is a counterpart of relatesTo(): it returns the list of (known, see the note) relations
    //! to the current event.
    //! \note This method uses `unsigned/m.relations` object that may not have fully accurate data.
    //!       Use with caution.
    QJsonObject relationsToThis() const;

    //! \brief Check whether there are other events relating to this
    //! \note This method uses `unsigned/m.relations` object that may not have fully accurate data.
    //!       Use with caution.
    bool hasRelationship(EventRelation::typeid_t relationTypeId) const;

    //! \brief Obtain id of an event replaced by the current one
    //! \sa RoomEvent::isReplaced, RoomEvent::replacedBy
    QString replacedEvent() const;

    //! \brief Determine whether the event has been replaced
    //!
    //! \return true if this event has been overridden by another event
    //!         with `"rel_type": "m.replace"`; false otherwise
    bool isReplaced() const;

    //! \brief Get the id of the event that replaced this one
    //!
    //! \return The id of the replacement event if the current event has been replaced
    //!         by another one; an empty string otherwise.
    //! \sa isReplaced, replacedEvent
    QString replacedBy() const;

    //! \brief Make a replaced event
    //!
    //! \returns a clone of `*this` with content taken from \p replacement as described in
    //!          https://spec.matrix.org/latest/client-server-api/#applying-mnew_content
    //! \note Disposal of the original event after that is on the caller.
    event_ptr_tt<RoomEvent> makeReplaced(const RoomEvent &replacementEvent) const;

    //! \brief Determine whether the event is part of a thread.
    //!
    //! \return true if this event is part of a thread, i.e. it has
    //!         `"rel_type": "m.thread"` or  `"m.relations": { "m.thread": {}}`;
    //!         false otherwise.
    bool isThreaded() const;

    //! \brief The event ID for the thread root event.
    //!
    //! \note This will return the ID of the event if it is the thread root.
    //!
    //! \note If the event is the thread root event and has not been updated with the server-side
    //!       the function will return an empty string as we can't tell if the message
    //!       is threaded.
    //!
    //! \return The event ID of the thread root if threaded, an empty string otherwise.
    QString threadRootEventId()const;

protected:
    explicit RoomEvent(const QJsonObject& json);
    void dumpTo(QDebug dbg) const override;

    virtual void afterRelationChange() {}

private:
    QString _id;

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
