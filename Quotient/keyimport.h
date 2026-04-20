// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "expected.h" // Only here to not break client code still using Expected
#include "quotient_export.h"

#include <QtCore/QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class TestKeyImport;

namespace Quotient
{
class Connection;
}

namespace Quotient
{

class QUOTIENT_API KeyImport : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Error {
        Success,
        InvalidPassphrase,
        InvalidData,
        OtherError,
    };
    Q_ENUM(Error)

    using QObject::QObject;

    Q_INVOKABLE Error importKeys(QString data, const QString& passphrase,
                                 Quotient::Connection* connection);
    Q_INVOKABLE std::expected<QByteArray, Error> exportKeys(const QString& passphrase,
                                                            Quotient::Connection* connection);

    friend class ::TestKeyImport;
};

}
