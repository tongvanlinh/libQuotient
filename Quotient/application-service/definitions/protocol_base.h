// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Definition of valid values for a field.
struct QUOTIENT_API FieldType
{
    //! A regular expression for validation of a field's value. This may be relatively
    //! coarse to verify the value as the application service providing this protocol
    //! may apply additional validation or filtering.
    QString regexp;

    //! A placeholder serving as a valid example of the field value.
    QString placeholder;
};

template <>
struct JsonObjectConverter<FieldType>
{
    static void dumpTo(QJsonObject &jo, const FieldType &pod)
    {
        addParam(jo, "regexp"_L1, pod.regexp);
        addParam(jo, "placeholder"_L1, pod.placeholder);
    }
    static void fillFrom(const QJsonObject &jo, FieldType &pod)
    {
        fillFromJson(jo.value("regexp"_L1), pod.regexp);
        fillFromJson(jo.value("placeholder"_L1), pod.placeholder);
    }
};

struct QUOTIENT_API ThirdPartyProtocol
{
    //! Fields which may be used to identify a third-party user. These should be
    //! ordered to suggest the way that entities may be grouped, where higher
    //! groupings are ordered first. For example, the name of a network should be
    //! searched before the nickname of a user.
    QStringList userFields;

    //! Fields which may be used to identify a third-party location. These should be
    //! ordered to suggest the way that entities may be grouped, where higher
    //! groupings are ordered first. For example, the name of a network should be
    //! searched before the name of a channel.
    QStringList locationFields;

    //! A content URI representing an icon for the third-party protocol.
    QString icon;

    //! The type definitions for the fields defined in `user_fields` and
    //! `location_fields`. Each entry in those arrays MUST have an entry here.
    //! The `string` key for this object is the field name itself.
    //!
    //! May be an empty object if no fields are defined.
    QHash<QString, FieldType> fieldTypes;
};

template <>
struct JsonObjectConverter<ThirdPartyProtocol>
{
    static void dumpTo(QJsonObject &jo, const ThirdPartyProtocol &pod)
    {
        addParam(jo, "user_fields"_L1, pod.userFields);
        addParam(jo, "location_fields"_L1, pod.locationFields);
        addParam(jo, "icon"_L1, pod.icon);
        addParam(jo, "field_types"_L1, pod.fieldTypes);
    }
    static void fillFrom(const QJsonObject &jo, ThirdPartyProtocol &pod)
    {
        fillFromJson(jo.value("user_fields"_L1), pod.userFields);
        fillFromJson(jo.value("location_fields"_L1), pod.locationFields);
        fillFromJson(jo.value("icon"_L1), pod.icon);
        fillFromJson(jo.value("field_types"_L1), pod.fieldTypes);
    }
};

} // namespace Quotient
