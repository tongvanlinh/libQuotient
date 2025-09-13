// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "mxcreply.h"

#include <QtCore/QBuffer>

#include "events/filesourceinfo.h"

using namespace Quotient;

class Q_DECL_HIDDEN MxcReply::Private
{
public:
    QNetworkReply* m_reply;
    QIODevice* m_device;
};

MxcReply::MxcReply(QNetworkReply* reply,
                   const EncryptedFileMetadata& fileMetadata)
    : d(makeImpl<Private>(reply, fileMetadata.isValid() ? nullptr : reply))
{
    reply->setParent(this);
    connect(d->m_reply, &QNetworkReply::finished, this, [this, fileMetadata] {
        setError(d->m_reply->error(), d->m_reply->errorString());

        if (d->m_reply->error() != NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(d->m_reply->readAll());
            QString errorString;
            if (doc.object().contains("error"_L1)) {
                errorString = doc["error"_L1].toString();
            } else {
                errorString = d->m_reply->errorString();
            }
            setErrorString(QStringLiteral("%1 (%2)").arg(errorString,
                                                         d->m_reply->url().toString()));
        } else if (fileMetadata.isValid()) {
            auto buffer = new QBuffer(this);
            buffer->setData(decryptFile(d->m_reply->readAll(), fileMetadata));
            buffer->open(ReadOnly);
            d->m_device = buffer;
        }
        setOpenMode(ReadOnly);
        emit finished();
    });
    connect(d->m_reply, &QNetworkReply::sslErrors, this, &MxcReply::sslErrors);
}

MxcReply::MxcReply()
    : d(ZeroImpl<Private>())
{
    static const auto BadRequestPhrase = tr("Bad Request");
    QMetaObject::invokeMethod(this, [this]() {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 400);
            setAttribute(QNetworkRequest::HttpReasonPhraseAttribute,
                         BadRequestPhrase);
            setError(QNetworkReply::ProtocolInvalidOperationError,
                     BadRequestPhrase);
            setFinished(true);
            emit errorOccurred(QNetworkReply::ProtocolInvalidOperationError);
            emit finished();
        }, Qt::QueuedConnection);
}

qint64 MxcReply::readData(char *data, qint64 maxlen)
{
    if(d != nullptr && d->m_device != nullptr) {
        return d->m_device->read(data, maxlen);
    }
    return -1;
}

void MxcReply::ignoreSslErrorsImplementation(const QList<QSslError> &ssl_errors)
{
    d->m_reply->ignoreSslErrors(ssl_errors);
}

void MxcReply::abort()
{
    if(d != nullptr && d->m_reply != nullptr) {
        d->m_reply->abort();
    }
}

qint64 MxcReply::bytesAvailable() const
{
    if (d != nullptr && d->m_device != nullptr) {
        return d->m_device->bytesAvailable() + QNetworkReply::bytesAvailable();
    }
    return 0;
}
