// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QTest>

#include <Quotient/settings.h>

using namespace Qt::Literals;
using Quotient::Settings, Quotient::SettingsGroup, Quotient::AccountSettings;

class TestSettings : public QObject {
    Q_OBJECT

public:
    static void initMain();

private slots:
    void initTestCase();
    void accountSettings_data() const;
    void accountSettings();

private:
    static inline const auto AccountsGroupName = u"Accounts"_s;
    QSettings qSettings{};
};

void TestSettings::initMain() { QCoreApplication::setOrganizationName(u"Quotient"_s); }

void TestSettings::initTestCase()
{
    qSettings.remove(AccountsGroupName);
}

void TestSettings::accountSettings_data() const
{
    QTest::addColumn<QString>("mxId");

    QTest::newRow("normal MXID") << u"@user:example.org"_s;
    QTest::newRow("MXID with slashes") << u"@user/with\\slashes:example.org"_s; // Test #842
}

void TestSettings::accountSettings()
{
    static const auto homeserverUrl = QUrl(u"https://example.org"_s);
    static const auto deviceName = u"SomeDevice"_s;
    QFETCH(QString, mxId);
    const auto escapedMxId = Settings::escapedForSettings(mxId);

    QVERIFY(SettingsGroup(AccountsGroupName).childGroups().empty()); // Pre-requisite
    qSettings.beginGroup(AccountsGroupName);
    { // Test writing to account settings
        AccountSettings accSettings(mxId);
        QCOMPARE(accSettings.userId(), mxId);
        QCOMPARE(accSettings.group(), AccountsGroupName % u'/' % escapedMxId);
        accSettings.setDeviceName(deviceName);
        QCOMPARE(accSettings.deviceName(), deviceName);
        accSettings.setHomeserver(homeserverUrl);
        QCOMPARE(accSettings.homeserver(), homeserverUrl);
    }

    qSettings.sync();
    // NB: QSettings::contains() doesn't work on groups, only on leaf keys; hence childGroups below
    auto childGroups = qSettings.childGroups();
    QVERIFY(childGroups.contains(escapedMxId));
    QVERIFY(SettingsGroup(AccountsGroupName).childGroups().contains(mxId));
    { // Test reading what was previously written
        SettingsGroup allAccountNames(AccountsGroupName);
        QCOMPARE(allAccountNames.childGroups().size(), 1);
        AccountSettings accSettings(allAccountNames.childGroups().back());
        QCOMPARE(accSettings.userId(), mxId);
        QCOMPARE(accSettings.deviceName(), deviceName);
        QCOMPARE(accSettings.homeserver(), homeserverUrl);
    }
    SettingsGroup(AccountsGroupName).remove(mxId); // Finally, test removal
    qSettings.sync();
    childGroups = qSettings.childGroups();
    QVERIFY(!childGroups.contains(escapedMxId));
    qSettings.endGroup();
}

QTEST_GUILESS_MAIN(TestSettings)
#include "testsettings.moc"
