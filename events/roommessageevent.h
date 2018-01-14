/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "event.h"

#include "eventcontent.h"

namespace QMatrixClient
{
    namespace MessageEventContent = EventContent; // Back-compatibility

    /**
     * The event class corresponding to m.room.message events
     */
    class RoomMessageEvent: public RoomEvent
    {
            Q_GADGET
            Q_PROPERTY(QString msgType READ rawMsgtype CONSTANT)
            Q_PROPERTY(QString plainBody READ plainBody CONSTANT)
            Q_PROPERTY(QMimeType mimeType READ mimeType STORED false CONSTANT)
            Q_PROPERTY(EventContent::TypedBase* content READ content CONSTANT)
        public:
            enum class MsgType
            {
                Text, Emote, Notice, Image, File, Location, Video, Audio, Unknown
            };

            RoomMessageEvent(const QString& plainBody,
                             const QString& jsonMsgType,
                             EventContent::TypedBase* content = nullptr)
                : RoomEvent(Type::RoomMessage)
                , _msgtype(jsonMsgType), _plainBody(plainBody), _content(content)
            { }
            explicit RoomMessageEvent(const QString& plainBody,
                                      MsgType msgType = MsgType::Text,
                                      EventContent::TypedBase* content = nullptr);
            explicit RoomMessageEvent(const QJsonObject& obj);

            MsgType msgtype() const;
            QString rawMsgtype() const       { return _msgtype; }
            const QString& plainBody() const { return _plainBody; }
            EventContent::TypedBase* content() const
                                             { return _content.data(); }
            QMimeType mimeType() const;
            bool hasFileContent() const;

            QJsonObject toJson() const;

            static constexpr const char* TypeId = "m.room.message";

        private:
            QString _msgtype;
            QString _plainBody;
            QScopedPointer<EventContent::TypedBase> _content;

            REGISTER_ENUM(MsgType)
    };
    using MessageEventType = RoomMessageEvent::MsgType;

    namespace EventContent
    {
        // Additional event content types

        /**
         * Rich text content for m.text, m.emote, m.notice
         *
         * Available fields: mimeType, body. The body can be either rich text
         * or plain text, depending on what mimeType specifies.
         */
        class TextContent: public TypedBase
        {
            public:
                TextContent(const QString& text, const QString& contentType);
                explicit TextContent(const QJsonObject& json);

                QMimeType type() const override { return mimeType; }

                QMimeType mimeType;
                QString body;

            protected:
                void fillJson(QJsonObject* json) const override;
        };

        /**
         * Content class for m.location
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - geoUri ("geo_uri" in JSON)
         * - corresponding to the "info" subobject:
         *   - thumbnail.url ("thumbnail_url" in JSON)
         * - corresponding to the "info/thumbnail_info" subobject:
         *   - thumbnail.payloadSize
         *   - thumbnail.mimeType
         *   - thumbnail.imageSize
         */
        class LocationContent: public TypedBase
        {
            public:
                LocationContent(const QString& geoUri,
                                const ImageInfo& thumbnail);
                explicit LocationContent(const QJsonObject& json);

                QMimeType type() const override;

            public:
                QString geoUri;
                Thumbnail thumbnail;

            protected:
                void fillJson(QJsonObject* o) const override;
        };

        /**
         * A base class for info types that include duration: audio and video
         */
        template <typename ContentT>
        class PlayableContent : public ContentT
        {
            public:
                PlayableContent(const QJsonObject& json)
                    : ContentT(json)
                    , duration(ContentT::originalInfoJson["duration"].toInt())
                { }

            protected:
                void fillJson(QJsonObject* json) const override
                {
                    ContentT::fillJson(json);
                    auto infoJson = json->take("info").toObject();
                    infoJson.insert("duration", duration);
                    json->insert("info", infoJson);
                }

            public:
                int duration;
        };

        /**
         * Content class for m.video
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - url
         *   - filename (extension to the CS API spec)
         * - corresponding to the "info" subobject:
         *   - payloadSize ("size" in JSON)
         *   - mimeType ("mimetype" in JSON)
         *   - duration
         *   - imageSize (QSize for a combination of "h" and "w" in JSON)
         *   - thumbnail.url ("thumbnail_url" in JSON)
         * - corresponding to the "info/thumbnail_info" subobject: contents of
         *   thumbnail field, in the same vein as for "info":
         *   - payloadSize
         *   - mimeType
         *   - imageSize
         */
        using VideoContent = PlayableContent<UrlWithThumbnailContent<ImageInfo>>;

        /**
         * Content class for m.audio
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - url
         *   - filename (extension to the CS API spec)
         * - corresponding to the "info" subobject:
         *   - payloadSize ("size" in JSON)
         *   - mimeType ("mimetype" in JSON)
         *   - duration
         */
        using AudioContent = PlayableContent<UrlBasedContent<FileInfo>>;
    }  // namespace EventContent
}  // namespace QMatrixClient
