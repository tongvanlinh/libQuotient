// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptionevent.h"

#include "../e2ee/e2ee_common.h"

using namespace Quotient;

EncryptionEventContent::EncryptionEventContent(const QJsonObject& json)
    : encryption(fromJson<Quotient::EncryptionType>(json[AlgorithmKeyL]))
    , algorithm(sanitized(json[AlgorithmKeyL].toString()))
{
    // NB: fillFromJson only fills the variable if the JSON key exists
    fillFromJson<int>(json[RotationPeriodMsKeyL], rotationPeriodMs);
    fillFromJson<int>(json[RotationPeriodMsgsKeyL], rotationPeriodMsgs);
}

EncryptionEventContent::EncryptionEventContent(Quotient::EncryptionType et)
    : encryption(et)
{
    if(encryption != Quotient::EncryptionType::Undefined) {
        algorithm = Quotient::toJson(encryption);
    }
}

QJsonObject EncryptionEventContent::toJson() const
{
    QJsonObject o;
    if (encryption != Quotient::EncryptionType::Undefined)
        o.insert(AlgorithmKey, algorithm);
    o.insert(RotationPeriodMsKey, rotationPeriodMs);
    o.insert(RotationPeriodMsgsKey, rotationPeriodMsgs);
    return o;
}
