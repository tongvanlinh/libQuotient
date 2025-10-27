// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QStringBuilder>

#include <ranges>

class QVariant;

namespace Quotient {

class QUOTIENT_API Settings : public QSettings {
    Q_OBJECT
public:
    /// Add a legacy organisation/application name to migrate settings from
    /*!
     * Use this function before creating any Settings objects in order
     * to set a legacy location where configuration has previously been stored.
     * This will provide an additional fallback in case of renaming
     * the organisation/application. Values in legacy locations are _removed_
     * when setValue() or remove() is called.
     */
    static void setLegacyNames(const QString& organizationName, const QString& applicationName = {});

    explicit Settings(QObject* parent = nullptr);

    /// Set the value for a given key
    /*! If the key exists in the legacy location, it is removed. */
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);

    /// Remove the value from both the primary and legacy locations
    Q_INVOKABLE void remove(const QString& key);

    /// Obtain a value for a given key
    /*!
     * If the key doesn't exist in the primary settings location, the legacy
     * location is checked. If neither location has the key,
     * \p defaultValue is returned.
     *
     * This function returns a QVariant; use get<>() to get the unwrapped
     * value if you know the type upfront.
     *
     * \sa setLegacyNames, get
     */
    Q_INVOKABLE QVariant value(const QString& key, const QVariant& defaultValue = {}) const;

    /// Obtain a value for a given key, coerced to the given type
    /*!
     * On top of value(), this function unwraps the QVariant and returns
     * its contents assuming the type passed as the template parameter.
     * If the type is different from the one stored inside the QVariant,
     * \p defaultValue is returned. In presence of legacy settings,
     * only the first found value is checked; if its type does not match,
     * further checks through legacy settings are not performed and
     * \p defaultValue is returned.
     */
    template <typename T>
    T get(const QString& key, const T& defaultValue = {}) const
    {
        const auto qv = value(key);
        return qv.isValid() && qv.canConvert<T>() ? qv.value<T>() : defaultValue;
    }

    Q_INVOKABLE bool contains(const QString& key) const;

    Q_INVOKABLE QStringList childGroups() const;
    Q_INVOKABLE QStringList childGroups(bool decodeSlashes) const;

    //! Escape forward- and backslashes in keys because QSettings doesn't (see #842)
    static QString toEncoded(QString key);

    //! Unescape `\` and `/` in keys stored with escapedForSettings()
    static QString fromEncoded(QString key);

private:
    static QString legacyOrganizationName;
    static QString legacyApplicationName;

protected:
    QSettings legacySettings { legacyOrganizationName, legacyApplicationName };
};

class QUOTIENT_API SettingsGroup : public Settings {
    Q_OBJECT
public:
    explicit SettingsGroup(const QString& path, QObject* parent = nullptr);
};

#define QUO_DECLARE_SETTING(type, propname, setter)      \
    Q_PROPERTY(type propname READ propname WRITE setter) \
public:                                                  \
    type propname() const;                               \
    void setter(type newValue);                          \
                                                         \
private:

#define QUO_DEFINE_SETTING(classname, type, propname, qsettingname, defaultValue, setter)   \
    type classname::propname() const { return get<type>(qsettingname##_L1, defaultValue); } \
    void classname::setter(type newValue) { setValue(qsettingname##_L1, std::move(newValue)); }

//! \brief A group of settings for one Matrix account
//!
//! This class provides typesafe accessors to common account settings such as user and device id.
//! User id (aka MXID) is stored as a group name. Although QSettings does not protect forward- and
//! backslashes inside group names, AccountSettings covers for that, percent-encoding the user id
//! before passing it to QSettings.
class QUOTIENT_API AccountSettings : public SettingsGroup {
    Q_OBJECT
    Q_PROPERTY(QString userId READ userId CONSTANT)
    QUO_DECLARE_SETTING(QString, deviceId, setDeviceId)
    QUO_DECLARE_SETTING(QString, deviceName, setDeviceName)
    QUO_DECLARE_SETTING(bool, keepLoggedIn, setKeepLoggedIn)
    Q_PROPERTY(QByteArray encryptionAccountPickle READ encryptionAccountPickle
                   WRITE setEncryptionAccountPickle)
public:
    explicit AccountSettings(const QString& accountId, QObject* parent = nullptr);

    QString userId() const;

    QUrl homeserver() const;
    void setHomeserver(const QUrl& url);

    [[deprecated("Client code shouldn't use the pickle; and the library stores it in a keychain")]]
    QByteArray encryptionAccountPickle();
    [[deprecated("Client code shouldn't use the pickle; and the library stores it in a keychain")]]
    void setEncryptionAccountPickle(const QByteArray& encryptionAccountPickle);
    Q_INVOKABLE void clearEncryptionAccountPickle();
};

class QUOTIENT_API AccountSettingsGroup : public SettingsGroup {
    Q_OBJECT
public:
    static auto name() { return u"Accounts"_s; }

    explicit AccountSettingsGroup(QObject* parent = nullptr);

    auto asRange() const
    {
        return std::views::transform(accountNames(),
                                     [](const QString& mxid) { return AccountSettings(mxid); });
    }

    //! \brief Obtain the list of child groups from the current or, if missing, legacy settings
    //! \note Slashes in account names will be automatically unescaped
    //! \sa AccountSettings
    Q_INVOKABLE QStringList accountNames() const;
};

} // namespace Quotient
