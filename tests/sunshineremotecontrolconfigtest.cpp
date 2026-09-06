#include "sunshineremotecontrolconfig.h"

#include <QTest>

using SunshineRemoteControlConfig::isViewOnly;
using SunshineRemoteControlConfig::withViewOnly;

class SunshineRemoteControlConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void isViewOnly_falseWhenKeysAbsent();
    void isViewOnly_falseWhenAnyKeyIsTrue();
    void isViewOnly_trueWhenAllThreeAreFalse();
    void isViewOnly_honorsLastOccurrenceOfARepeatedKey();
    void withViewOnly_addsAllThreeKeysWhenEnabling();
    void withViewOnly_removesKeysWhenDisabling();
    void withViewOnly_preservesUnrelatedConfigLines();
    void withViewOnly_roundTrips();

private:
    static const QString baseConfig;
};

const QString SunshineRemoteControlConfigTest::baseConfig = QStringLiteral("port = 47989\nlog_path = /tmp/sunshine.log\n");

void SunshineRemoteControlConfigTest::isViewOnly_falseWhenKeysAbsent()
{
    QVERIFY(!isViewOnly(baseConfig));
}

void SunshineRemoteControlConfigTest::isViewOnly_falseWhenAnyKeyIsTrue()
{
    QVERIFY(!isViewOnly(baseConfig + QStringLiteral("mouse = false\nkeyboard = true\ncontroller = false\n")));
}

void SunshineRemoteControlConfigTest::isViewOnly_trueWhenAllThreeAreFalse()
{
    QVERIFY(isViewOnly(baseConfig + QStringLiteral("mouse = false\nkeyboard = false\ncontroller = false\n")));
}

void SunshineRemoteControlConfigTest::isViewOnly_honorsLastOccurrenceOfARepeatedKey()
{
    // Sunshine (and Moonbeam's own port parsing) takes the last occurrence
    // when a key is repeated.
    const QString config = baseConfig + QStringLiteral("mouse = false\nkeyboard = false\ncontroller = false\nmouse = true\n");
    QVERIFY(!isViewOnly(config));
}

void SunshineRemoteControlConfigTest::withViewOnly_addsAllThreeKeysWhenEnabling()
{
    const QString result = withViewOnly(baseConfig, true);
    QVERIFY(isViewOnly(result));
    QVERIFY(result.contains(QStringLiteral("port = 47989")));
}

void SunshineRemoteControlConfigTest::withViewOnly_removesKeysWhenDisabling()
{
    const QString viewOnlyConfig = baseConfig + QStringLiteral("mouse = false\nkeyboard = false\ncontroller = false\n");
    const QString result = withViewOnly(viewOnlyConfig, false);
    QVERIFY(!isViewOnly(result));
    QVERIFY(!result.contains(QStringLiteral("mouse")));
    QVERIFY(!result.contains(QStringLiteral("keyboard")));
    QVERIFY(!result.contains(QStringLiteral("controller")));
    QVERIFY(result.contains(QStringLiteral("port = 47989")));
}

void SunshineRemoteControlConfigTest::withViewOnly_preservesUnrelatedConfigLines()
{
    const QString result = withViewOnly(baseConfig, true);
    QVERIFY(result.contains(QStringLiteral("log_path = /tmp/sunshine.log")));
}

void SunshineRemoteControlConfigTest::withViewOnly_roundTrips()
{
    QVERIFY(isViewOnly(withViewOnly(baseConfig, true)));
    QVERIFY(!isViewOnly(withViewOnly(withViewOnly(baseConfig, true), false)));
}

QTEST_GUILESS_MAIN(SunshineRemoteControlConfigTest)
#include "sunshineremotecontrolconfigtest.moc"
