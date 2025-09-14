// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "../logging_categories_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QStringBuilder>

using namespace Quotient;

namespace {
std::pair<const AbstractEventMetaType *, event_type_t> findFirstOverlap(
    std::span<const AbstractEventMetaType *const> metaTypes,
    std::span<event_type_t const> matrixTypeIds)
{
    using namespace std::ranges;
    for (const auto *metaType : metaTypes)
        if (const auto overlappingIdIt = find_first_of(matrixTypeIds, metaType->matrixIds);
            overlappingIdIt != end(matrixTypeIds))
            return {metaType, *overlappingIdIt};
    return {};
}
}

AbstractEventMetaType::AbstractEventMetaType(const std::type_info &typeInfo, const char *className,
                                             AbstractEventMetaType *nearestBase,
                                             std::vector<event_type_t> matrixTypeIds)
    : typeInfo(typeInfo)
    , className(className)
    , baseType(nearestBase)
    , matrixIds(std::move(matrixTypeIds))
{
    if (nearestBase) {
        if (const auto overlap = findFirstOverlap(nearestBase->derivedTypes(), matrixIds);
            overlap.first)
        {
            if (QUO_ALARM_X(overlap.first == this, "Attempt to re-register the same event class"))
                return; // This is kinda fine but extremely fishy

            // Two different metatype objects claim the same Matrix type id; this
            // is not normal, so give as much information as possible to diagnose
            if (QUO_ALARM_X(overlap.first->typeInfo == typeInfo,
                            QLatin1StringView(className) % " claims '"_L1 % overlap.second
                                % "' repeatedly; check that the C++ symbol is properly exported"_L1))
                return; // That situation is very wrong (see #413) so maybe std::terminate() even?

            qWarning(EVENTS).nospace() << overlap.second << " is already mapped to "
                                       << overlap.first->className << " before " << className
                                       << "; unless the two have different isValid() conditions, "
                                          "the latter class will never be used";
        }
        nearestBase->_derivedTypes.emplace_back(this);
        qDebug(EVENTS).nospace().noquote()
            << std::format("{:n:s}", matrixIds) << " -> " << className << "; "
            << nearestBase->_derivedTypes.size() << " derived type(s) registered for "
            << nearestBase->className;
    }
}

Event::Event(const QJsonObject& json) : _json(json) {}

Event::~Event() = default;

QString Event::matrixType() const { return fullJson()[TypeKey].toString(); }

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKey].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKey].toObject();
}

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
