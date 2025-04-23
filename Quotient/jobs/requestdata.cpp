// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "requestdata.h"

#include <QtCore/QIODevice>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

using namespace Quotient;

auto bufferFromData(const QByteArray& data)
{
    auto source = makeImpl<QBuffer, QIODevice>();
    source->setData(data);
    source->open(QIODevice::ReadOnly);
    return source;
}

template <typename JsonDataT>
    requires std::constructible_from<QJsonDocument, JsonDataT>
inline auto bufferFromJson(const JsonDataT& jdata)
{
    return bufferFromData(QJsonDocument(jdata).toJson(QJsonDocument::Compact));
}

RequestData::RequestData(const QByteArray& a) : _source(bufferFromData(a)) {}

RequestData::RequestData(const QJsonObject& jo) : _source(bufferFromJson(jo)) {}

RequestData::RequestData(const QJsonArray& ja) : _source(bufferFromJson(ja)) {}

RequestData::RequestData(QIODevice* source)
    : _source(acquireImpl(source))
{}
