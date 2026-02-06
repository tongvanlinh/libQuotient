// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testutils.h"

#include <Quotient/room.h>
#include <Quotient/networkaccessmanager.h>

#include <QtTest/QSignalSpy>

using Quotient::Connection, Quotient::Room;

std::shared_ptr<Connection> Quotient::createTestConnection(QLatin1StringView localUserName,
                                                           QLatin1StringView secret,
                                                           QLatin1StringView deviceName)
{
    static constexpr auto homeserverAddr = "localhost:1234"_L1;
    auto* const nam = NetworkAccessManager::instance();
    QObject::connect(nam, &QNetworkAccessManager::sslErrors, nam,
                     [](QNetworkReply* reply) { reply->ignoreSslErrors(); });

    auto c = std::make_shared<Connection>();
    c->enableEncryption(true);
    const QString userId{ u'@' % localUserName % u':' % homeserverAddr };
    c->setHomeserver(QUrl(u"https://" % homeserverAddr));
    if (!waitForSignal(c, &Connection::loginFlowsChanged)
        || !c->supportsPasswordAuth()) {
        qCritical().noquote() << "Can't use password login at" << homeserverAddr
                              << "- check that the homeserver is running";
        return nullptr;
    }
    c->loginWithPassword(localUserName, secret, deviceName);
    if (!waitForSignal(c, &Connection::connected)) {
        qCritical().noquote()
            << "Could not achieve the logged in state for" << userId
            << "- check the credentials in the test code and at the homeserver";
        return nullptr;
    }
    return c;
}

std::pair<Room *, Room *> Quotient::createTestChat(Connection *c1, Connection *c2)
{
    auto c2RoomFuture = c1->createDirectChat(c2->userId())
                            .then([c2](const QString &roomId) {
        return c2->joinAndGetRoom(roomId);
    }).unwrap();
    if (!waitForFuture(c2RoomFuture)) {
        qCritical("Couldn't set up a test direct chat - check the logs above");
        return {nullptr, nullptr};
    }
    auto c2Room = c2RoomFuture.result();
    if (!c2Room || !c2Room->id().startsWith(u'!') || c2Room->joinState() != JoinState::Join) {
        qCritical("Invalid room returned for the joined user");
        return {nullptr, nullptr};
    }
    auto c1Room = c1->room(c2Room->id());
    if (!c1Room || c1Room->id() != c2Room->id())
        qFatal("Room views of the two members don't match");
    return {c1Room, c2Room};
}
