// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomevent.h"

#include "encryptedevent.h"
#include "redactionevent.h"
#include "roomcreateevent.h"
#include "roommemberevent.h"
#include "roompowerlevelsevent.h"
#include "stateevent.h"

#include "../logging_categories_p.h"

using namespace Quotient;

RoomEvent::RoomEvent(const QJsonObject& json) : Event(json)
{
    if (const auto redaction = unsignedPart<QJsonObject>(RedactedCauseKey);
        !redaction.isEmpty())
        _redactedBecause = loadEvent<RedactionEvent>(redaction);
}

RoomEvent::~RoomEvent() = default; // Let the smart pointer do its job

QString RoomEvent::displayId() const { return id().isEmpty() ? transactionId() : id(); }

QString RoomEvent::id() const { return fullJson()[EventIdKey].toString(); }

QDateTime RoomEvent::originTimestamp() const
{
    return Quotient::fromJson<QDateTime>(fullJson()["origin_server_ts"_L1]);
}

QString RoomEvent::roomId() const
{
    return fullJson()[RoomIdKey].toString();
}

QString RoomEvent::senderId() const
{
    return fullJson()[SenderKey].toString();
}

QString RoomEvent::redactionReason() const
{
    return isRedacted() ? _redactedBecause->reason() : QString {};
}

event_ptr_tt<RoomEvent> RoomEvent::makeRedacted(const RedactionEvent &redaction) const
{
    // The logic below faithfully follows the spec despite quite a few of
    // the preserved keys being only relevant for homeservers. Just in case.
    static const QStringList TopLevelKeysToKeep{
        EventIdKey,  TypeKey,          RoomIdKey,        SenderKey,
        StateKeyKey, ContentKey,       "hashes"_L1,      "signatures"_L1,
        "depth"_L1,  "prev_events"_L1, "auth_events"_L1, "origin_server_ts"_L1
    };

    auto originalJson = this->fullJson();
    for (auto it = originalJson.begin(); it != originalJson.end();) {
        if (!TopLevelKeysToKeep.contains(it.key()))
            it = originalJson.erase(it);
        else
            ++it;
    }
    if (!this->is<RoomCreateEvent>()) { // See MSC2176 on create events
        static const QHash<QString, QStringList> ContentKeysToKeepPerType{
            { RedactionEvent::TypeId, { "redacts"_L1 } },
            { RoomMemberEvent::TypeId,
              { "membership"_L1, "join_authorised_via_users_server"_L1 } },
            { RoomPowerLevelsEvent::TypeId,
              { "ban"_L1, "events"_L1, "events_default"_L1, "invite"_L1,
                "kick"_L1, "redact"_L1, "state_default"_L1, "users"_L1,
                "users_default"_L1 } },
            // TODO: Replace with RoomJoinRules::TypeId etc. once available
            { "m.room.join_rules"_L1, { "join_rule"_L1, "allow"_L1 } },
            { "m.room.history_visibility"_L1, { "history_visibility"_L1 } }
        };

        if (const auto contentKeysToKeep = ContentKeysToKeepPerType.value(this->matrixType());
            !contentKeysToKeep.isEmpty()) {
            editSubobject(originalJson, ContentKey, [&contentKeysToKeep](QJsonObject& content) {
                for (auto it = content.begin(); it != content.end();) {
                    if (!contentKeysToKeep.contains(it.key()))
                        it = content.erase(it);
                    else
                        ++it;
                }
            });
        } else {
            originalJson.remove(ContentKey);
            originalJson.remove(PrevContentKey);
        }
    }
    replaceSubvalue(originalJson, UnsignedKey, RedactedCauseKey, redaction.fullJson());

    return loadEvent<RoomEvent>(originalJson);
}

QString RoomEvent::transactionId() const
{
    return unsignedPart<QString>("transaction_id"_L1);
}

bool RoomEvent::isStateEvent() const { return is<StateEvent>(); }

QString RoomEvent::stateKey() const
{
    return fullJson()[StateKeyKey].toString();
}

void RoomEvent::setRoomId(const QString& roomId)
{
    editJson().insert(RoomIdKey, roomId);
}

void RoomEvent::setSender(const QString& senderId)
{
    editJson().insert(SenderKey, senderId);
}

void RoomEvent::setTransactionId(const QString& txnId)
{
    auto unsignedData = fullJson()[UnsignedKey].toObject();
    unsignedData.insert("transaction_id"_L1, txnId);
    editJson().insert(UnsignedKey, unsignedData);
    Q_ASSERT(transactionId() == txnId);
}

void RoomEvent::addId(const QString& newId)
{
    Q_ASSERT(id().isEmpty());
    Q_ASSERT(!newId.isEmpty());
    editJson().insert(EventIdKey, newId);
    _id = newId;
    qCDebug(EVENTS) << "Event txnId -> id:" << transactionId() << "->" << id();
    Q_ASSERT(id() == newId);
}

void RoomEvent::dumpTo(QDebug dbg) const
{
    Event::dumpTo(dbg);
    dbg << " (made at " << originTimestamp().toString(Qt::ISODate) << ')';
}

void RoomEvent::setOriginalEvent(event_ptr_tt<EncryptedEvent>&& originalEvent)
{
    _originalEvent = std::move(originalEvent);
}

const QJsonObject RoomEvent::encryptedJson() const
{
    if(!_originalEvent) {
        return {};
    }
    return _originalEvent->fullJson();
}

namespace {
bool containsEventType(const auto& haystack, const auto& needle)
{
    return std::ranges::any_of(haystack, [needle](const AbstractEventMetaType* candidate) {
        return candidate->matrixId == needle || containsEventType(candidate->derivedTypes(), needle);
    });
}
}

bool Quotient::isStateEvent(const QString& eventTypeId)
{
    return containsEventType(StateEvent::BaseMetaType.derivedTypes(), eventTypeId);
}

event_ptr_tt<RoomEvent> RoomEvent::makeReplaced(const RoomEvent &replacement) const
{
    // See https://spec.matrix.org/latest/client-server-api/#applying-mnew_content
    auto newContent = replacement.contentPart<QJsonObject>(NewContentKey);
    addParam<IfNotEmpty>(newContent, RelatesToKey, this->contentPart<QJsonObject>(RelatesToKey));
    auto originalJson = this->fullJson();
    originalJson.insert(ContentKey, newContent);
    editSubobject(originalJson, UnsignedKey, [&replacement](QJsonObject &unsignedData) {
        replaceSubvalue(unsignedData, RelationsKey, EventRelation::ReplacementType,
                        replacement.id());
    });

    return loadEvent<RoomEvent>(originalJson);
}

