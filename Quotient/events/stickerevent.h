// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"
#include "eventcontent.h"

namespace Quotient {

/// Sticker messages are specialised image messages that are displayed without
/// controls (e.g. no "download" link, or light-box view on click, as would be
/// displayed for for m.image events).
class QUOTIENT_API StickerEvent
    : public EventTemplate<StickerEvent, RoomEvent, EventContent::ImageContent>
{
public:
    QUO_EVENT(StickerEvent, "m.sticker")

    StickerEvent(const QString &body, content_type imageContent)
        : EventTemplate(imageContent), m_imageContent(std::move(imageContent))
    {
        replaceSubvalue(editJson(), ContentKey, BodyKey, body);
    }

    /// \brief A textual representation or associated description of the
    /// sticker image.
    ///
    /// This could be the alt text of the original image, or a message to
    /// accompany and further describe the sticker.
    QUO_CONTENT_GETTER(QString, body)

    /// \brief Metadata about the image referred to in url including a
    /// thumbnail representation.
    const EventContent::ImageContent& image() const
    {
        if (!m_imageContent)
            m_imageContent.emplace(content());
        return *m_imageContent;
    }

    /// \brief The URL to the sticker image. This must be a valid mxc:// URI.
    QUrl url() const
    {
        return image().url();
    }

protected:
    explicit StickerEvent(const QJsonObject &json) : EventTemplate(json) {}

private:
    mutable std::optional<EventContent::ImageContent> m_imageContent;
};
} // namespace Quotient
