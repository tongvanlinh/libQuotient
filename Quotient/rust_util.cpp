// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "rust_util.h"

#include "ranges_extras.h"

#include <QtCore/QJsonDocument>

QUOTIENT_API rust::Vec<rust::String> Quotient::stringsToRust(const QStringList& strings)
{
    return rangeTo<rust::Vec<rust::String>>(std::ranges::transform_view(strings, &stringToRust));
}
QUOTIENT_API QJsonObject Quotient::jsonFromRust(const rust::String& string)
{
    return QJsonDocument::fromJson(bytesFromRust(string)).object();
}
