// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "util.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtCore/QtEndian>

namespace Quotient { // To avoid having to prepend Quotient:: to all the free functions below

inline constexpr auto RegExpOptions =
    QRegularExpression::CaseInsensitiveOption
    | QRegularExpression::UseUnicodePropertiesOption;

// Converts all that looks like a URL into HTML links
void linkifyUrls(QString& htmlEscapedText)
{
    // Note: outer parentheses are a part of C++ raw string delimiters, not of
    // the regex (see http://en.cppreference.com/w/cpp/language/string_literal).
    // Note2: the next-outer parentheses are \N in the replacement.

    // generic url:
    // regexp is originally taken from Konsole (https://github.com/KDE/konsole)
    // protocolname:// or www. followed by anything other than whitespaces,
    // <, >, ' or ", and ends before whitespaces, <, >, ', ", ], !, ), :,
    // comma or dot
    static const QRegularExpression FullUrlRegExp(
        uR"(\b((www\.(?!\.)(?!(\w|\.|-)+@)|(https?|ftp):(//)?\w|(magnet|matrix):)(&(?![lg]t;)|[^&\s<>'"])+(&(?![lg]t;)|[^&!,.\s<>'"\]):])))"_s,
        RegExpOptions);
    static const QRegularExpression EmailAddressRegExp(
        uR"((^|[][[:space:](){}`'";<>])(mailto:)?((\w|[!#$%&'*+=^_‘{|}~.-])+@(\w|\.|-)+\.\w+\b))"_s,
        RegExpOptions);
    // An interim liberal implementation of
    // https://matrix.org/docs/spec/appendices.html#identifier-grammar
    static const QRegularExpression MxIdRegExp(
        uR"((^|[][[:space:](){}`'";])([!#@][-a-z0-9_=#/.]{1,252}:\w(?:\w|\.|-)*\.\w+(?::\d{1,5})?))"_s,
        RegExpOptions);
    Q_ASSERT(FullUrlRegExp.isValid() && EmailAddressRegExp.isValid() && MxIdRegExp.isValid());

    // NOTE: htmlEscapedText is already HTML-escaped! No literal <,>,&,"

    htmlEscapedText.replace(EmailAddressRegExp, uR"(\1<a href='mailto:\3'>\2\3</a>)"_s);
    htmlEscapedText.replace(FullUrlRegExp, uR"(<a href='\1'>\1</a>)"_s);
    htmlEscapedText.replace(MxIdRegExp, uR"(\1<a href='https://matrix.to/#/\2'>\2</a>)"_s);
}

QString sanitized(const QString& plainText)
{
    auto text = plainText;
    text.remove(QChar(0x202e)); // RLO
    text.remove(QChar(0x202d)); // LRO
    text.remove(QChar(0xfffc)); // Object replacement character
    return text;
}

QString prettyPrint(const QString& plainText)
{
    auto pt = plainText.toHtmlEscaped();
    linkifyUrls(pt);
    pt.replace(u'\n', "<br/>"_L1);
    return "<span style='white-space:pre-wrap'>"_L1 % pt % "</span>"_L1;
}

QString cacheLocation(QStringView dirName)
{
    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u'/'
        % dirName % u'/';
    if (const QDir dir(cachePath); !dir.exists())
        dir.mkpath("."_L1);
    return cachePath;
}

qreal stringToHueF(const QString& s)
{
    Q_ASSERT(!s.isEmpty());
    const auto hash =
        QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha1);
    QDataStream dataStream(hash.left(2));
    dataStream.setByteOrder(QDataStream::LittleEndian);
    quint16 hashValue = 0;
    dataStream >> hashValue;
    const auto hueF = qreal(hashValue) / std::numeric_limits<quint16>::max();
    Q_ASSERT((0 <= hueF) && (hueF <= 1));
    return hueF;
}

inline constexpr auto ServerPartRegEx =
    "(\\[[^][:space:]]+]|[-[:alnum:].]+)" // IPv6 address or hostname/IPv4 address
    "(?::(\\d{1,5}))?"_L1; // Optional port

QString serverPart(const QString& mxId)
{
    static const QString re("^[@!#$+].*?:("_L1 // Localpart and colon
                            % ServerPartRegEx % ")$"_L1);
    static const QRegularExpression parser(
        re,
        QRegularExpression::UseUnicodePropertiesOption); // Because Asian digits
    Q_ASSERT(parser.isValid());
    return parser.match(mxId).captured(1);
}

QString versionString() { return QStringLiteral(Quotient_VERSION_STRING); }

int majorVersion() { return Quotient_VERSION_MAJOR; }

int minorVersion() { return Quotient_VERSION_MINOR; }

int patchVersion() { return Quotient_VERSION_PATCH; }

bool isGuestUserId(const UserId& uId)
{
    static const QRegularExpression guestMxIdRe{ u"^@\\d+:"_s };
    return guestMxIdRe.match(uId).hasMatch();
}

bool HomeserverData::checkMatrixSpecVersion(QStringView targetVersion) const
{
    // TODO: Replace this naïve implementation with something smarter that can check things like
    //   1.12 > 1.11 and maybe even 1.10 > 1.9
    return std::ranges::any_of(supportedSpecVersions, [targetVersion](const QString& v) {
        return v.startsWith(targetVersion);
    });
}
}
