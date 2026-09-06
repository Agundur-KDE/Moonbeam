#include "sunshineportconfig.h"

#include <QTest>

using SunshinePortConfig::resolveWebUiPort;

class SunshinePortConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsWhenNoPortLine();
    void addsOneToConfiguredPort();
    void honorsLastOccurrenceOfARepeatedKey();
    void invalidForNonNumericValue();
    void invalidForNegativeValue();
    void invalidWhenAdditionWouldOverflow();
    void validAtTheHighestSafeValue();
};

void SunshinePortConfigTest::defaultsWhenNoPortLine()
{
    const auto port = resolveWebUiPort(QStringLiteral("log_path = /tmp/x\n"));
    QVERIFY(port.has_value());
    QCOMPARE(*port, quint16(47990));
}

void SunshinePortConfigTest::addsOneToConfiguredPort()
{
    const auto port = resolveWebUiPort(QStringLiteral("port = 12345\n"));
    QVERIFY(port.has_value());
    QCOMPARE(*port, quint16(12346));
}

void SunshinePortConfigTest::honorsLastOccurrenceOfARepeatedKey()
{
    const auto port = resolveWebUiPort(QStringLiteral("port = 100\nport = 200\n"));
    QVERIFY(port.has_value());
    QCOMPARE(*port, quint16(201));
}

void SunshinePortConfigTest::invalidForNonNumericValue()
{
    QVERIFY(!resolveWebUiPort(QStringLiteral("port = not-a-number\n")).has_value());
}

void SunshinePortConfigTest::invalidForNegativeValue()
{
    QVERIFY(!resolveWebUiPort(QStringLiteral("port = -1\n")).has_value());
}

void SunshinePortConfigTest::invalidWhenAdditionWouldOverflow()
{
    // The old code cast an unchecked toUInt() to quint16 and added 1,
    // wrapping 65535 to 0 - the exact overflow this must now refuse.
    QVERIFY(!resolveWebUiPort(QStringLiteral("port = 65535\n")).has_value());
    QVERIFY(!resolveWebUiPort(QStringLiteral("port = 99999999999\n")).has_value());
}

void SunshinePortConfigTest::validAtTheHighestSafeValue()
{
    const auto port = resolveWebUiPort(QStringLiteral("port = 65534\n"));
    QVERIFY(port.has_value());
    QCOMPARE(*port, quint16(65535));
}

QTEST_GUILESS_MAIN(SunshinePortConfigTest)
#include "sunshineportconfigtest.moc"
