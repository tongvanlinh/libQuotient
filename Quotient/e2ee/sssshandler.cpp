// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "sssshandler.h"

using namespace Quotient;

void SSSSHandler::unlockSSSSWithPassphrase(const QString& passphrase)
{
    m_connection->loadFromBackup(passphrase);
}

void SSSSHandler::unlockSSSSFromCrossSigning()
{
    m_connection->requestSecretsFromDevices();
}

Connection* SSSSHandler::connection() const
{
    return m_connection;
}

void SSSSHandler::setConnection(Connection* connection)
{
    if (connection == m_connection) {
        return;
    }
    m_connection = connection;
    emit connectionChanged();

    connect(connection, &Connection::backupFinished, this, [this](const auto &status){
        if (status == Connection::BackupResult::Success) {
            emit keyBackupUnlocked();
        } else {
            emit error(status == Connection::BackupResult::InvalidPassphrase ? WrongKeyError : DecryptionError);
        }
    });
}

void SSSSHandler::unlockSSSSFromSecurityKey(const QString& encodedKey)
{
    m_connection->loadFromBackup(encodedKey);
}
