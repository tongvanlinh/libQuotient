// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QTest>

#include <Quotient/settings.h>

using namespace Qt::Literals;
using Quotient::Settings, Quotient::SettingsGroup, Quotient::AccountSettings;

class TestSettings : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void accountSettings();

private:
    static inline const auto AccountsGroupName = u"Accounts"_s;
    QSettings qSettings{};
};

void TestSettings::initTestCase()
{
    qSettings.remove(AccountsGroupName);
}

void TestSettings::accountSettings()
{
    const auto mxId = u"@user/with\\slashes:example.org"_s; // Test #842
    const auto escapedMxId = Settings::escapedForSettings(mxId);
    const auto homeserverUrl = QUrl(u"https://example.org"_s);

    qSettings.beginGroup(AccountsGroupName);
    {
        AccountSettings accSettings(mxId);
        accSettings.setHomeserver(homeserverUrl);
        QVERIFY(accSettings.homeserver() == homeserverUrl);
        // Bypass SettingsGroup::get() that prepends the group name and check that the group name
        // has actually been prepended.
        QVERIFY(accSettings.Settings::get<QUrl>(AccountsGroupName % u'/' % escapedMxId % u'/'
                                                % u"homeserver"_s)
                == homeserverUrl);
    }

    qSettings.sync();
    // NB: QSettings::contains() doesn't work on groups, only on leaf keys; hence childGroups below
    auto childGroups = qSettings.childGroups();
    QVERIFY(childGroups.contains(escapedMxId));
    QVERIFY(SettingsGroup(AccountsGroupName).childGroups().contains(mxId));
    SettingsGroup(AccountsGroupName).remove(mxId);
    qSettings.sync();
    childGroups = qSettings.childGroups();
    QVERIFY(!childGroups.contains(escapedMxId));
    qSettings.endGroup();
}

QTEST_GUILESS_MAIN(TestSettings)
#include "testsettings.moc"
