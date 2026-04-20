// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "filesourceinfo.h"

#include "../e2ee/e2ee_common.h"
#include "../logging_categories_p.h"
#include "../rust_util.h"

#include <QtCore/QReadWriteLock>
#include <QtCore/QCryptographicHash>

using namespace Quotient;

QByteArray Quotient::decryptFile(const QByteArray& ciphertext,
                                 const EncryptedFileMetadata& metadata)
{
    if (QByteArray::fromBase64(metadata.hashes["sha256"_L1].toLatin1())
        != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return {};
    }

    const auto result = crypto::decrypt_file(toRustSlice<byte_t>(ciphertext),
                                             stringToRust(metadata.iv), stringToRust(metadata.key.k),
                                             stringToRust(metadata.hashes["sha256"_L1]));
    if (result->has_error()) {
        auto error = result->error_string();
        qCWarning(E2EE) << stringFromRust(error);
        return {};
    }

    return bytesFromRust(result->value());
}

std::pair<EncryptedFileMetadata, QByteArray> Quotient::encryptFile(QByteArray plainText)
{
    auto info = crypto::encrypt_file(toRustSlice<byte_t>(plainText));
    EncryptedFileMetadata metadata {
        .url = {},
        .key = {
            "oct"_L1, { "encrypt"_L1, "decrypt"_L1 }, "A256CTR"_L1, stringFromRust(info->media_encryption_info_key()), true
        },
        .iv = stringFromRust(info->media_encryption_info_iv()),
        .hashes = {{u"sha256"_s, stringFromRust(info->media_encryption_info_hash())}},
        .v = u"v2"_s,
    };
    return { metadata, bytesFromRust(info->media_encryption_info_bytes()) };
}

void JsonObjectConverter<EncryptedFileMetadata>::dumpTo(
    QJsonObject& jo, const EncryptedFileMetadata& pod)
{
    addParam(jo, "url"_L1, pod.url);
    addParam(jo, "key"_L1, pod.key);
    addParam(jo, "iv"_L1, pod.iv);
    addParam(jo, "hashes"_L1, pod.hashes);
    addParam(jo, "v"_L1, pod.v);
}

void JsonObjectConverter<EncryptedFileMetadata>::fillFrom(
    const QJsonObject& jo, EncryptedFileMetadata& pod)
{
    fromJson(jo.value("url"_L1), pod.url);
    fromJson(jo.value("key"_L1), pod.key);
    fromJson(jo.value("iv"_L1), pod.iv);
    fromJson(jo.value("hashes"_L1), pod.hashes);
    fromJson(jo.value("v"_L1), pod.v);
}

void JsonObjectConverter<JWK>::dumpTo(QJsonObject& jo, const JWK& pod)
{
    addParam(jo, "kty"_L1, pod.kty);
    addParam(jo, "key_ops"_L1, pod.keyOps);
    addParam(jo, "alg"_L1, pod.alg);
    addParam(jo, "k"_L1, pod.k);
    addParam(jo, "ext"_L1, pod.ext);
}

void JsonObjectConverter<JWK>::fillFrom(const QJsonObject& jo, JWK& pod)
{
    fromJson(jo.value("kty"_L1), pod.kty);
    fromJson(jo.value("key_ops"_L1), pod.keyOps);
    fromJson(jo.value("alg"_L1), pod.alg);
    fromJson(jo.value("k"_L1), pod.k);
    fromJson(jo.value("ext"_L1), pod.ext);
}

namespace {
template <typename FsiT>
auto& getUrl(FsiT& fsi)
{
    return std::visit( // std::variant_alternative_t<> applies const from FsiT if it's there
        Overloads{ [](std::variant_alternative_t<0, FsiT>& url) -> auto& { return url; },
                   [](std::variant_alternative_t<1, FsiT>& efm) -> auto& { return efm.url; } },
        fsi);
}
}

QUrl Quotient::getUrlFromSourceInfo(const FileSourceInfo& fsi) { return getUrl(fsi); }

void Quotient::setUrlInSourceInfo(FileSourceInfo& fsi, const QUrl& newUrl) { getUrl(fsi) = newUrl; }

void Quotient::fillJson(QJsonObject& jo, const FileSourceInfoKeys& jsonKeys,
                        const FileSourceInfo& fsi)
{
    jo.insert(jsonKeys[fsi.index()], toJson(fsi));
}

FileSourceInfo Quotient::fileSourceInfoFromJson(const QJsonObject& jo,
                                                const FileSourceInfoKeys& jsonKeys)
{
    return jo.contains(jsonKeys[1]) ? fromJson<EncryptedFileMetadata>(jo[jsonKeys[1]])
                                    : FileSourceInfo(fromJson<QUrl>(jo[jsonKeys[0]]));
}

namespace {
    // A map from roomId/eventId pair to file source info
    QHash<std::pair<QString, QString>, EncryptedFileMetadata> infos;
    QReadWriteLock lock;
}

void FileMetadataMap::add(const QString& roomId, const QString& eventId,
                          const EncryptedFileMetadata& fileMetadata)
{
    const QWriteLocker l(&lock);
    infos.insert({ roomId, eventId }, fileMetadata);
}

void FileMetadataMap::remove(const QString& roomId, const QString& eventId)
{
    const QWriteLocker l(&lock);
    infos.remove({ roomId, eventId });
}

EncryptedFileMetadata FileMetadataMap::lookup(const QString& roomId,
                                              const QString& eventId)
{
    const QReadLocker l(&lock);
    return infos.value({ roomId, eventId });
}
