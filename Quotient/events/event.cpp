// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "../logging_categories_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QStringBuilder>

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR <= 9
#include "stateevent.h" // For deprecated isStateEvent(); remove, once Event::isStateEvent() is gone
#endif

using namespace Quotient;

AbstractEventMetaType::AbstractEventMetaType(const std::type_info &typeInfo, const char *className,
                                             AbstractEventMetaType *nearestBase,
                                             event_type_t matrixId)
    : typeInfo(typeInfo), className(className), baseType(nearestBase), matrixId(matrixId)
{
    if (nearestBase) {
        if (const auto existing = std::ranges::find(nearestBase->derivedTypes(), matrixId,
                                                    &AbstractEventMetaType::matrixId);
            existing != nearestBase->derivedTypes().end()) // macOS still has no std::span::cend()
        {
            if (QUO_ALARM_X(*existing == this, "Attempt to re-register the same event class"))
                return; // This is kinda fine but extremely fishy

            // Two different metatype objects claim the same Matrix type id; this
            // is not normal, so give as much information as possible to diagnose
            if (QUO_ALARM_X((*existing)->typeInfo == typeInfo,
                            QLatin1StringView(className) % " claims '"_L1 % matrixId
                                % "' repeatedly; check that the C++ symbol is properly exported"_L1))
                return; // That situation is very wrong (see #413) so maybe std::terminate() even?

            qWarning(EVENTS).nospace() << matrixId << " is already mapped to "
                                       << (*existing)->className << " before " << className
                                       << "; unless the two have different isValid() conditions, "
                                          "the latter class will never be used";
        }
        nearestBase->_derivedTypes.emplace_back(this);
        qDebug(EVENTS).nospace() << matrixId << " -> " << className << "; "
                                 << nearestBase->_derivedTypes.size()
                                 << " derived type(s) registered for " << nearestBase->className;
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

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR <= 9
bool Event::isStateEvent() const { return is<StateEvent>(); }
#endif

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
