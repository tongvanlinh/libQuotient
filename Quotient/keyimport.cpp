// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "keyimport.h"

#include "connection.h"

using namespace Quotient;

KeyImport::Error KeyImport::importKeys(QString data, const QString& passphrase, Connection* connection)
{
    return connection->importKeys(passphrase, data);
}


std::expected<QByteArray, KeyImport::Error> KeyImport::exportKeys(const QString& passphrase, Connection* connection)
{
    return connection->exportKeys(passphrase);
}
