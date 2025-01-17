// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "settings.h"

#include "logging_categories_p.h"

#include "ranges_extras.h"

#include <QtCore/QUrl>

using namespace Quotient;

QString Settings::legacyOrganizationName {};
QString Settings::legacyApplicationName {};

void Settings::setLegacyNames(const QString& organizationName,
                              const QString& applicationName)
{
    legacyOrganizationName = organizationName;
    legacyApplicationName = applicationName;
}

QString Settings::toEncoded(QString key)
{
    key.replace(u'/', u"%2F"_s);
    key.replace(u'\\', u"%5C"_s);
    return key;
}

QString Settings::fromEncoded(QString key)
{
    key.replace(u"%2F"_s, u"/"_s);
    key.replace(u"%5C"_s, u"\\"_s);
    return key;
}

Settings::Settings(QObject* parent) : QSettings(parent)
{}

void Settings::setValue(const QString& key, const QVariant& value)
{
    QSettings::setValue(key, value);
    if (legacySettings.contains(key))
        legacySettings.remove(key);
}

void Settings::remove(const QString& key)
{
    auto safeKey = group() == u"Accounts"_s ? toEncoded(key) : key;
    QSettings::remove(safeKey);
    legacySettings.remove(safeKey);
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    auto value = QSettings::value(key, legacySettings.value(key, defaultValue));
    // QML's Qt.labs.Settings stores boolean values as strings, which, if loaded
    // through the usual QSettings interface, confuses QML
    // (QVariant("false") == true in JavaScript). Since we have a mixed
    // environment where both QSettings and Qt.labs.Settings may potentially
    // work with same settings, better ensure compatibility.
    return value.toString() == "false"_L1 ? QVariant(false) : value;
}

bool Settings::contains(const QString& key) const
{
    return QSettings::contains(key) || legacySettings.contains(key);
}

QStringList Settings::childGroups() const { return childGroups(true); }

QStringList Settings::childGroups(bool decodeSlashes) const
{
    auto groups = QSettings::childGroups();
    const auto& legacyGroups = legacySettings.childGroups();
    for (const auto& g: legacyGroups)
        if (!groups.contains(g))
            groups.push_back(g);
    if (group() == u"Accounts" && decodeSlashes) {
        qWarning(MAIN)
            << "Developers, use AccountSettingsGroup to work with the Accounts/ group of settings";
        std::ranges::for_each(groups, [](QString& g) { g = fromEncoded(g); }); // See #842
    }
    return groups;
}

SettingsGroup::SettingsGroup(const QString& path, QObject* parent)
    : Settings(parent)
{
    beginGroup(path);
    legacySettings.beginGroup(path);
}

QUO_DEFINE_SETTING(AccountSettings, QString, deviceId, "device_id", {},
                   setDeviceId)
QUO_DEFINE_SETTING(AccountSettings, QString, deviceName, "device_name", {},
                   setDeviceName)
QUO_DEFINE_SETTING(AccountSettings, bool, keepLoggedIn, "keep_logged_in", false,
                   setKeepLoggedIn)

namespace {
constexpr auto HomeserverKey = "homeserver"_L1;
constexpr auto EncryptionAccountPickleKey = "encryption_account_pickle"_L1;
}

QUrl AccountSettings::homeserver() const
{
    return QUrl::fromUserInput(value(HomeserverKey).toString());
}

void AccountSettings::setHomeserver(const QUrl& url)
{
    setValue(HomeserverKey, url.toString());
}

AccountSettings::AccountSettings(const QString& accountId, QObject* parent)
    : SettingsGroup(AccountSettingsGroup::name() % u'/' % toEncoded(accountId), parent)
{}

QString AccountSettings::userId() const { return fromEncoded(group().section(u'/', -1)); }

QByteArray AccountSettings::encryptionAccountPickle()
{
    return value("encryption_account_pickle"_L1, QString()).toByteArray();
}

void AccountSettings::setEncryptionAccountPickle(
    const QByteArray& encryptionAccountPickle)
{
    setValue("encryption_account_pickle"_L1, encryptionAccountPickle);
}

void AccountSettings::clearEncryptionAccountPickle()
{
    remove(EncryptionAccountPickleKey); // TODO: Force to re-issue it?
}

AccountSettingsGroup::AccountSettingsGroup(QObject* parent) : SettingsGroup(name(), parent) {}

QStringList AccountSettingsGroup::accountNames() const
{
    return rangeTo<QStringList>(std::views::transform(childGroups(false), fromEncoded)); // See #842
}
