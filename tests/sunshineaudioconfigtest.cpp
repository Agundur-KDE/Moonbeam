#include "sunshineaudioconfig.h"

#include <QTest>

using SunshineAudioConfig::isSurroundEnabled;
using SunshineAudioConfig::withSurroundEnabled;

class SunshineAudioConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void isSurroundEnabled_falseWhenKeyAbsent();
    void isSurroundEnabled_falseWhenPointingElsewhere();
    void isSurroundEnabled_trueWhenPointingAtSurroundMonitor();
    void isSurroundEnabled_honorsLastOccurrenceOfARepeatedKey();
    void withSurroundEnabled_addsKeyWhenEnabling();
    void withSurroundEnabled_removesKeyWhenDisabling();
    void withSurroundEnabled_preservesUnrelatedConfigLines();
    void withSurroundEnabled_roundTrips();

private:
    static const QString baseConfig;
};

const QString SunshineAudioConfigTest::baseConfig = QStringLiteral("port = 47989\nlog_path = /tmp/sunshine.log\n");

void SunshineAudioConfigTest::isSurroundEnabled_falseWhenKeyAbsent()
{
    QVERIFY(!isSurroundEnabled(baseConfig));
}

void SunshineAudioConfigTest::isSurroundEnabled_falseWhenPointingElsewhere()
{
    QVERIFY(!isSurroundEnabled(baseConfig + QStringLiteral("audio_sink = analog-stereo.monitor\n")));
}

void SunshineAudioConfigTest::isSurroundEnabled_trueWhenPointingAtSurroundMonitor()
{
    QVERIFY(isSurroundEnabled(baseConfig + QStringLiteral("audio_sink = moonbeam-surround.monitor\n")));
}

void SunshineAudioConfigTest::isSurroundEnabled_honorsLastOccurrenceOfARepeatedKey()
{
    const QString config = baseConfig + QStringLiteral("audio_sink = moonbeam-surround.monitor\naudio_sink = analog-stereo.monitor\n");
    QVERIFY(!isSurroundEnabled(config));
}

void SunshineAudioConfigTest::withSurroundEnabled_addsKeyWhenEnabling()
{
    const QString result = withSurroundEnabled(baseConfig, true);
    QVERIFY(isSurroundEnabled(result));
    QVERIFY(result.contains(QStringLiteral("port = 47989")));
}

void SunshineAudioConfigTest::withSurroundEnabled_removesKeyWhenDisabling()
{
    const QString enabledConfig = baseConfig + QStringLiteral("audio_sink = moonbeam-surround.monitor\n");
    const QString result = withSurroundEnabled(enabledConfig, false);
    QVERIFY(!isSurroundEnabled(result));
    QVERIFY(!result.contains(QStringLiteral("audio_sink")));
    QVERIFY(result.contains(QStringLiteral("port = 47989")));
}

void SunshineAudioConfigTest::withSurroundEnabled_preservesUnrelatedConfigLines()
{
    const QString result = withSurroundEnabled(baseConfig, true);
    QVERIFY(result.contains(QStringLiteral("log_path = /tmp/sunshine.log")));
}

void SunshineAudioConfigTest::withSurroundEnabled_roundTrips()
{
    QVERIFY(isSurroundEnabled(withSurroundEnabled(baseConfig, true)));
    QVERIFY(!isSurroundEnabled(withSurroundEnabled(withSurroundEnabled(baseConfig, true), false)));
}

QTEST_GUILESS_MAIN(SunshineAudioConfigTest)
#include "sunshineaudioconfigtest.moc"
