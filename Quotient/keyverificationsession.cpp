// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationsession.h"

#include <QTimer>

#include "connection.h"
#include "connection_p.h"
#include "room.h"

using namespace Quotient;

KeyVerificationSession::KeyVerificationSession(const QString& remoteUserId, const QString& verificationId, const QString& remoteDeviceId, Connection* connection)
    : QObject(connection)
    , m_remoteUserId(remoteUserId)
    , m_verificationId(verificationId)
    , m_remoteDeviceId(remoteDeviceId)
    , m_connection(connection)
    , m_processTimer(new QTimer)
{
    if (verificationId.isEmpty()) {
        setState(CREATED);
        m_connection->d->requestDeviceVerification(this);
    }

    startMonitoring();

    connect(this, &KeyVerificationSession::stateChanged, this, [this](){
        if (m_state == DONE || m_state == CANCELLED) {
            deleteLater();
        }
    });
}

KeyVerificationSession::KeyVerificationSession(Room* room, Connection* connection, const QString& verificationId)
    : QObject(connection)
    , m_verificationId(verificationId)
    , m_connection(connection)
    , m_room(room)
    , m_processTimer(new QTimer)
{
    const auto& ids = room->joinedMemberIds();
    if (QUO_ALARM_X(ids.size() != 2, "Refusing to perform key verification in a room with"_L1
                                         % QString::number(ids.size()) % "members"_L1))
        return;

    m_remoteUserId = ids[0] == connection->userId() ? ids[1] : ids[0];

    if (m_verificationId.isEmpty()) {
        setState(CREATED);
        m_connection->d->requestUserVerification(this);
    } else {
        setState(REQUESTED);
        startMonitoring();
    }
}

KeyVerificationSession* KeyVerificationSession::requestDeviceVerification(const QString& userId, const QString& deviceId, Connection* connection)
{
    return new KeyVerificationSession(userId, {}, deviceId, connection);
}

KeyVerificationSession* KeyVerificationSession::requestUserVerification(Room* room, Connection* connection)
{
    return new KeyVerificationSession(room, connection);
}


void KeyVerificationSession::accept()
{
    m_connection->d->acceptKeyVerification(this);
}

void KeyVerificationSession::confirm()
{
    m_connection->d->confirmKeyVerification(this);
}

void KeyVerificationSession::setState(State state)
{
    if (m_state == state) {
        return;
    }

    if (state == NOTFOUND) {
        return;
    }

    m_state = state;
    Q_EMIT stateChanged();
}

void KeyVerificationSession::setSasState(SasState state)
{
    if (m_sasState == state || state == SASNOTFOUND) {
        return;
    }

    m_sasState = state;
    Q_EMIT sasStateChanged();

    if (m_sasState == SASKEYSEXCHANGED) {
        emit sasEmojisChanged();
    }

    if (m_sasState == SASSTARTED && !weStarted) {
        m_connection->d->acceptSas(this);
    }
}

QList<EmojiEntry> KeyVerificationSession::sasEmojis()
{
    auto raw = m_connection->d->keyVerificationSasEmoji(this);

    QList<EmojiEntry> out;
    for (const auto& [symbol, description] : raw) {
        out += {
            symbol, description
        };
    }
    return out;
}

QString KeyVerificationSession::remoteDeviceId() const
{
    return m_remoteDeviceId;
}

void KeyVerificationSession::setVerificationId(const QString& verificationId)
{
    m_verificationId = verificationId;
}

QString KeyVerificationSession::remoteUser() const
{
    return m_remoteUserId;
}

QString KeyVerificationSession::verificationId() const
{
    return m_verificationId;
}

Room* KeyVerificationSession::room() const
{
    return m_room;
}

KeyVerificationSession* KeyVerificationSession::processIncomingUserVerification(Room* room, const QString& eventId)
{
    return new KeyVerificationSession(room, room->connection(), eventId);
}

void KeyVerificationSession::startSas()
{
    if (m_state != READY) {
        return;
    }
    weStarted = true;
    m_connection->d->startKeyVerification(this);
}

KeyVerificationSession* KeyVerificationSession::selfVerification(const QString& verificationId, Connection* connection)
{
    return new KeyVerificationSession(connection->userId(), verificationId, {}, connection);
}

void KeyVerificationSession::sendReady()
{
    accept();
}

void KeyVerificationSession::sendMac()
{
    confirm();
}

void KeyVerificationSession::sendStartSas()
{
    startSas();
}

void KeyVerificationSession::cancelVerification()
{
    m_connection->d->cancelKeyVerification(this);
}

void KeyVerificationSession::startMonitoring()
{
    m_connection->d->monitorVerification(this);
    connect(&Dispatcher::instance(), &Dispatcher::sessionChanged, this,
        [this](const auto &ourUserId, const auto &theirUserId, const auto &verificationId) {
            if (ourUserId == m_connection->userId() && theirUserId == m_remoteUserId
                && verificationId == m_verificationId) {
                setState(m_connection->d->keyVerificationSessionState(this));
            setSasState(m_connection->d->sasState(this));

            if (m_state == TRANSITIONED && !sasMonitorStarted) {
                sasMonitorStarted = true;
                m_connection->d->monitorSas(this);
            }
        }
    });
}
