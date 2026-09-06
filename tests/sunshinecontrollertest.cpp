#include "sunshinecontroller.h"

#include "fakes/fakesunshinenetworkprobe.h"
#include "fakes/fakesunshineprocess.h"

#include <QFile>
#include <QLockFile>
#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QTemporaryDir>
#include <QTest>

namespace
{
QString fixturePath(const QString &name)
{
    return QStringLiteral(MOONBEAM_TEST_FIXTURE_DIR "/") + name;
}

QSslCertificate loadCert(const QString &name)
{
    QFile file(fixturePath(name));
    Q_UNUSED(file.open(QIODevice::ReadOnly));
    return QSslCertificate(file.readAll(), QSsl::Pem);
}

void writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    Q_UNUSED(file.open(QIODevice::WriteOnly));
    file.write(content);
}

// A configDir with the real fixture certificate installed at the path
// SunshineIdentity resolves by default, so pinnedCertificateFingerprint()
// (and therefore isSunshineAt()) has something real to pin against.
QString makeConfigDirWithPinnedCert(QTemporaryDir &dir)
{
    const QString configDir = dir.path();
    QDir().mkpath(configDir + QStringLiteral("/credentials"));
    QFile::copy(fixturePath(QStringLiteral("real.pem")), configDir + QStringLiteral("/credentials/cacert.pem"));
    return configDir;
}
}

class SunshineControllerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void notInstalledWhenExecutableMissing();
    void stoppedWhenExecutableFoundAndPortClosed();
    void configInvalidWhenPortOutOfRange();
    void runningExternalWhenPortOpenAndCertMatchesPin();
    void notRunningExternalWhenCertHasSameCnDifferentKey_certImitation();
    void notRunningExternalWhenPortOpenButNoPinnableCert();
    void startSpawnsProcessWhenStopped();
    void startRefusesWhenLockIsHeldByAnotherProcess_parallelProcesses();
    void stopTerminatesThenKillsOnTimeout_hungProcess();
    void processCrashTriggersRefresh();
    void webUiUrlReflectsConfiguredPort();
    void webUiUrlEmptyWhenConfigInvalid();
    void setSurroundAudioWritesConfigWhileStopped();
    void setSurroundAudioIgnoredWhileSharing();
};

void SunshineControllerTest::notInstalledWhenExecutableMissing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QString();
                                   });

    QCOMPARE(controller.state(), SunshineController::State::NotInstalled);
}

void SunshineControllerTest::stoppedWhenExecutableFoundAndPortClosed()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QCOMPARE(controller.state(), SunshineController::State::Stopped);
    QVERIFY(controller.canStart());
}

void SunshineControllerTest::configInvalidWhenPortOutOfRange()
{
    // audit.txt S-06 at the controller level: an out-of-range configured
    // port must refuse to start, not guess.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), "port = 65535\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QCOMPARE(controller.state(), SunshineController::State::ConfigInvalid);
    QVERIFY(!controller.canStart());
}

void SunshineControllerTest::runningExternalWhenPortOpenAndCertMatchesPin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir);

    auto probe = std::make_unique<FakeSunshineNetworkProbe>();
    probe->portOpen = true;
    probe->certificateAtPort = loadCert(QStringLiteral("real.pem"));

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::move(probe),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QCOMPARE(controller.state(), SunshineController::State::RunningExternal);
    QVERIFY(!controller.canStop());
}

void SunshineControllerTest::notRunningExternalWhenCertHasSameCnDifferentKey_certImitation()
{
    // The S-01 exploit shape, reproduced at the controller level: a local
    // impostor's certificate shares Sunshine's exact CN but was issued
    // with a different key. Must not be accepted as RunningExternal.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir);

    auto probe = std::make_unique<FakeSunshineNetworkProbe>();
    probe->portOpen = true;
    probe->certificateAtPort = loadCert(QStringLiteral("imposter.pem"));

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::move(probe),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QVERIFY(controller.state() != SunshineController::State::RunningExternal);
}

void SunshineControllerTest::notRunningExternalWhenPortOpenButNoPinnableCert()
{
    // A "wrong/different port" scenario: something is listening, but there
    // is no certificate to pin against at all (e.g. a plain HTTP service),
    // so peerCertificateAt() reports null - must fail closed.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir);

    auto probe = std::make_unique<FakeSunshineNetworkProbe>();
    probe->portOpen = true;
    probe->certificateAtPort = QSslCertificate();

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::move(probe),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QVERIFY(controller.state() != SunshineController::State::RunningExternal);
}

void SunshineControllerTest::startSpawnsProcessWhenStopped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto processOwned = std::make_unique<FakeSunshineProcess>();
    FakeSunshineProcess *process = processOwned.get();

    SunshineController controller(std::move(processOwned),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QCOMPARE(controller.state(), SunshineController::State::Stopped);
    controller.start();

    QCOMPARE(process->startCallCount, 1);
    QCOMPARE(process->lastProgram, QStringLiteral("/usr/bin/sunshine-fake"));
    QCOMPARE(controller.state(), SunshineController::State::RunningOwned);
}

void SunshineControllerTest::startRefusesWhenLockIsHeldByAnotherProcess_parallelProcesses()
{
    // audit.txt S-02-style scenario at the controller's own start lock:
    // another Moonbeam process already holds it, so this one must not
    // also spawn Sunshine.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString lockPath = dir.filePath(QStringLiteral("start.lock"));

    QLockFile heldByAnotherProcess(lockPath);
    QVERIFY(heldByAnotherProcess.tryLock(1000));

    auto processOwned = std::make_unique<FakeSunshineProcess>();
    FakeSunshineProcess *process = processOwned.get();

    SunshineController controller(std::move(processOwned),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   lockPath,
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   },
                                   200);

    QCOMPARE(controller.state(), SunshineController::State::Stopped);
    controller.start();

    QCOMPARE(process->startCallCount, 0);
}

void SunshineControllerTest::stopTerminatesThenKillsOnTimeout_hungProcess()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto processOwned = std::make_unique<FakeSunshineProcess>();
    processOwned->finishesOnTerminate = false; // simulate a hung process
    FakeSunshineProcess *process = processOwned.get();

    SunshineController controller(std::move(processOwned),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    controller.start();
    QCOMPARE(controller.state(), SunshineController::State::RunningOwned);

    controller.stop();

    QCOMPARE(process->terminateCallCount, 1);
    QCOMPARE(process->killCallCount, 1);
}

void SunshineControllerTest::processCrashTriggersRefresh()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto processOwned = std::make_unique<FakeSunshineProcess>();
    FakeSunshineProcess *process = processOwned.get();

    SunshineController controller(std::move(processOwned),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    controller.start();
    QCOMPARE(controller.state(), SunshineController::State::RunningOwned);

    QSignalSpy stateChangedSpy(&controller, &SunshineController::stateChanged);

    // Simulate the process crashing on its own: state() flips to
    // NotRunning first (as QProcess would report), then the signal fires.
    process->m_state = QProcess::NotRunning;
    Q_EMIT process->errorOccurred();

    QVERIFY(!stateChangedSpy.isEmpty());
    QCOMPARE(controller.state(), SunshineController::State::Stopped);
}

void SunshineControllerTest::webUiUrlReflectsConfiguredPort()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), "port = 12345\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QCOMPARE(controller.webUiUrl(), QStringLiteral("https://127.0.0.1:12346"));
}

void SunshineControllerTest::webUiUrlEmptyWhenConfigInvalid()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), "port = 65535\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QVERIFY(controller.webUiUrl().isEmpty());
}

void SunshineControllerTest::setSurroundAudioWritesConfigWhileStopped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QVERIFY(!controller.surroundAudio());
    controller.setSurroundAudio(true);
    QVERIFY(controller.surroundAudio());
}

void SunshineControllerTest::setSurroundAudioIgnoredWhileSharing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   dir.path(),
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    controller.start();
    QCOMPARE(controller.state(), SunshineController::State::RunningOwned);

    controller.setSurroundAudio(true);

    QVERIFY(!controller.surroundAudio());
}

QTEST_GUILESS_MAIN(SunshineControllerTest)
#include "sunshinecontrollertest.moc"
