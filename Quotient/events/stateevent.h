// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

constexpr inline auto PrevContentKey = "prev_content"_L1;

class QUOTIENT_API StateEvent : public RoomEvent {
public:
    QUO_BASE_EVENT(StateEvent, RoomEvent, "json.contains('state_key')")

    static bool isValid(const QJsonObject& fullJson)
    {
        return fullJson.contains(StateKeyKey);
    }

    //! \brief Static setting of whether a given even type uses state keys
    //!
    //! Most event types don't use a state key; overriding this to `true`
    //! for a given type changes the calls across Quotient to include state key
    //! in their signatures; otherwise, state key is still accessible but
    //! constructors and calls in, e.g., RoomStateView don't include it.
    static constexpr auto needsStateKey = false;

    explicit StateEvent(event_type_t type, const QString& stateKey = {},
                        const QJsonObject& contentJson = {});

    //! Make a minimal correct Matrix state event JSON
    static QJsonObject basicJson(const QString& matrixTypeId,
                                 const QString& stateKey = {},
                                 const QJsonObject& contentJson = {})
    {
        return { { TypeKey, matrixTypeId },
                 { StateKeyKey, stateKey },
                 { ContentKey, contentJson } };
    }

    QString replacedState() const;
    virtual bool repeatsState() const;

protected:
    explicit StateEvent(const QJsonObject& json);
    void dumpTo(QDebug dbg) const override;
};
using StateEventPtr = event_ptr_tt<StateEvent>;
using StateEvents = EventsArray<StateEvent>;

/**
 * A combination of event type and state key uniquely identifies a piece
 * of state in Matrix.
 * \sa
 * https://matrix.org/docs/spec/client_server/unstable.html#types-of-room-events
 */
using StateEventKey = std::pair<QString, QString>;

template <typename EventT, typename ContentT>
class EventTemplate<EventT, StateEvent, ContentT>
    : public StateEvent {
public:
    using content_type = ContentT;

    explicit EventTemplate(const QJsonObject &fullJson) : StateEvent(fullJson) {}

    template <typename... ContentParamTs>
    explicit EventTemplate(const QString &stateKey, ContentParamTs &&...contentParams)
        : StateEvent(EventT::TypeId, stateKey,
                     toJson(ContentT{std::forward<ContentParamTs>(contentParams)...}))
    {}

    ContentT content() const { return fromJson<ContentT>(Event::contentJson()); }

    template <std::invocable<ContentT &> VisitorT>
    void editContent(VisitorT &&visitor)
    {
        editContentJson([&visitor](QJsonObject &contentJson) mutable {
            auto content = fromJson<ContentT>(contentJson);
            std::invoke(std::forward<VisitorT>(visitor), content);
            contentJson = toJson(content);
        });
    }
    std::optional<ContentT> prevContent() const
    {
        return unsignedPart<std::optional<ContentT>>(PrevContentKey);
    }
    QString prevSenderId() const { return unsignedPart<QString>("prev_sender"_L1); }
};

template <typename EventT, typename ContentT>
class KeyedStateEventBase
    : public EventTemplate<EventT, StateEvent, ContentT> {
public:
    static constexpr auto needsStateKey = true;

    using EventTemplate<EventT, StateEvent, ContentT>::EventTemplate;
};

template <typename EvT>
concept Keyed_State_Event = EvT::needsStateKey;

template <typename EventT, typename ContentT>
class KeylessStateEventBase
    : public EventTemplate<EventT, StateEvent, ContentT> {
private:
    using base_type = EventTemplate<EventT, StateEvent, ContentT>;

public:
    template <typename... ContentParamTs>
        // Ideally, we want to check std::constructible_from<ContentT, ContentParamTs...> -
        // unfortunately, Xcode 15.4 still thinks that, e.g., AliasEventContent is not constructible
        // from QString and QStringList, so we have to make the check slightly indirect
        requires std::constructible_from<base_type, QString, ContentParamTs...>
    explicit KeylessStateEventBase(ContentParamTs&&... contentParams)
        : base_type(QString(), std::forward<ContentParamTs>(contentParams)...)
    {}

protected:
    explicit KeylessStateEventBase(const QJsonObject& fullJson)
        : base_type(fullJson)
    {}
};

template <typename EvT>
concept Keyless_State_Event = !EvT::needsStateKey;

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::StateEvent*)
Q_DECLARE_METATYPE(const Quotient::StateEvent*)

// https://stackoverflow.com/questions/68320024/why-did-the-c-standards-committee-not-include-stdhash-for-pair-and-tuple
template <>
struct std::hash<Quotient::StateEventKey> {
    size_t operator()(const Quotient::StateEventKey& k) const
    {
        return qHash(k, QHashSeed::globalSeed());
    }
};
