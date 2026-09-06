#include "sunshinecontroller.h"

#include "fakes/fakesunshinenetworkprobe.h"
#include "fakes/fakesunshineprocess.h"

#include <QFile>
#include <QLockFile>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include <unistd.h>

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

QSslKey loadKey(const QString &name)
{
    QFile file(fixturePath(name));
    Q_UNUSED(file.open(QIODevice::ReadOnly));
    return QSslKey(file.readAll(), QSsl::Ec);
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

/**
 * A minimal TLS+HTTP stand-in for Sunshine's web UI, used to prove
 * SunshineController::pair() (audit.txt S-01) never lets HTTP data -
 * credentials included - reach a server presenting anything other than the
 * pinned certificate. Drives the handshake manually (like
 * sunshinepinningtest.cpp's FakeSunshineServer) rather than via QSslServer.
 */
class FakeHttpsServer : public QObject
{
public:
    FakeHttpsServer(QSslCertificate cert, QSslKey key)
        : m_cert(std::move(cert))
        , m_key(std::move(key))
    {
        m_listening = m_tcpServer.listen(QHostAddress::LocalHost);
    }

    bool isListening() const
    {
        return m_listening;
    }

    QString errorString() const
    {
        return m_tcpServer.errorString();
    }

    quint16 port() const
    {
        return m_tcpServer.serverPort();
    }

    // Accepts one connection, completes the TLS handshake, reads one HTTP
    // request, and answers with a minimal successful pairing response.
    // Returns false if the handshake never completes within the timeout -
    // proof the client aborted before any HTTP request (and therefore the
    // Authorization header) could have been sent, since TLS application
    // data cannot exist before the handshake finishes on both ends.
    bool serveOnePairingRequest(QByteArray *receivedAuthorizationHeader = nullptr)
    {
        if (!m_tcpServer.hasPendingConnections()) {
            QSignalSpy newConnectionSpy(&m_tcpServer, &QTcpServer::newConnection);
            if (!newConnectionSpy.wait(2000)) {
                return false;
            }
        }

        QTcpSocket *raw = m_tcpServer.nextPendingConnection();
        if (!raw) {
            return false;
        }

        QSslSocket tls;
        tls.setLocalCertificate(m_cert);
        tls.setPrivateKey(m_key);
        tls.setSocketDescriptor(::dup(raw->socketDescriptor()), QAbstractSocket::ConnectedState, QAbstractSocket::ReadWrite);
        raw->deleteLater();

        QSignalSpy encryptedSpy(&tls, &QSslSocket::encrypted);
        tls.startServerEncryption();
        if (!tls.isEncrypted() && !encryptedSpy.wait(2000)) {
            return false;
        }

        QByteArray request;
        while (!request.contains("\r\n\r\n")) {
            if (tls.bytesAvailable() == 0 && !tls.waitForReadyRead(2000)) {
                return false;
            }
            request += tls.readAll();
        }

        if (receivedAuthorizationHeader) {
            static const QRegularExpression authLine(QStringLiteral("Authorization:\\s*(.+)\r\n"), QRegularExpression::CaseInsensitiveOption);
            const auto match = authLine.match(QString::fromUtf8(request));
            if (match.hasMatch()) {
                *receivedAuthorizationHeader = match.captured(1).trimmed().toUtf8();
            }
        }

        static const QByteArray responseBody = QByteArrayLiteral("{\"status\":true}");
        const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(responseBody.size())
            + "\r\nConnection: close\r\n\r\n" + responseBody;
        tls.write(response);
        tls.waitForBytesWritten(2000);
        tls.disconnectFromHost();
        return true;
    }

private:
    QTcpServer m_tcpServer;
    QSslCertificate m_cert;
    QSslKey m_key;
    bool m_listening = false;
};
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
    void pairSendsCredentialsWhenCertMatchesPin();
    void pairAbortsWithoutSendingCredentialsToImposterCertificate();
    void pairRejectsCertificateThatIsGloballyTrustedButNotPinned();
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

void SunshineControllerTest::pairSendsCredentialsWhenCertMatchesPin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir);

    FakeHttpsServer server(loadCert(QStringLiteral("real.pem")), loadKey(QStringLiteral("real.key")));
    QVERIFY2(server.isListening(), qPrintable(server.errorString()));
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), QByteArrayLiteral("port = ") + QByteArray::number(server.port() - 1) + "\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QSignalSpy succeededSpy(&controller, &SunshineController::pairingSucceeded);
    QSignalSpy failedSpy(&controller, &SunshineController::pairingFailed);

    controller.pair(QStringLiteral("1234"), QStringLiteral("Test Device"), QStringLiteral("moonbeam"), QStringLiteral("secret"));

    QByteArray authHeader;
    QVERIFY2(server.serveOnePairingRequest(&authHeader), "handshake against the pinned certificate must succeed");
    QVERIFY2(succeededSpy.wait(2000), qPrintable(failedSpy.isEmpty() ? QStringLiteral("no signal fired") : failedSpy.at(0).at(0).toString()));
    QVERIFY(failedSpy.isEmpty());
    QVERIFY(authHeader.contains("Basic"));
}

void SunshineControllerTest::pairAbortsWithoutSendingCredentialsToImposterCertificate()
{
    // audit.txt S-01 exploit shape: same CN as the pinned cert, different
    // key. Must never let the Authorization header reach this server.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir); // pins real.pem

    FakeHttpsServer server(loadCert(QStringLiteral("imposter.pem")), loadKey(QStringLiteral("imposter.key")));
    QVERIFY2(server.isListening(), qPrintable(server.errorString()));
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), QByteArrayLiteral("port = ") + QByteArray::number(server.port() - 1) + "\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QSignalSpy succeededSpy(&controller, &SunshineController::pairingSucceeded);
    QSignalSpy failedSpy(&controller, &SunshineController::pairingFailed);

    controller.pair(QStringLiteral("1234"), QStringLiteral("Test Device"), QStringLiteral("moonbeam"), QStringLiteral("secret"));

    QVERIFY2(!server.serveOnePairingRequest(), "the client must abort the TLS handshake before any HTTP data can reach an impostor certificate");
    QVERIFY(!failedSpy.isEmpty() || failedSpy.wait(2000));
    QCOMPARE(succeededSpy.count(), 0);
}

void SunshineControllerTest::pairRejectsCertificateThatIsGloballyTrustedButNotPinned()
{
    // Proves pinning is enforced structurally, not only when Qt happens to
    // raise sslErrors: this certificate is added to the process-wide
    // default trust store first, so a naive VerifyPeer implementation that
    // checks the fingerprint only inside an sslErrors handler would see
    // zero errors here and never run that check at all (audit.txt S-01,
    // reviewer-requested third case).
    const QSslCertificate otherTrustedCert = loadCert(QStringLiteral("othertrusted.pem"));
    const QSslConfiguration originalDefaultConfig = QSslConfiguration::defaultConfiguration();
    auto restoreDefaultConfig = qScopeGuard([originalDefaultConfig] {
        QSslConfiguration::setDefaultConfiguration(originalDefaultConfig);
    });
    QSslConfiguration defaultConfig = originalDefaultConfig;
    defaultConfig.addCaCertificate(otherTrustedCert);
    QSslConfiguration::setDefaultConfiguration(defaultConfig);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configDir = makeConfigDirWithPinnedCert(dir); // still pins real.pem, not otherTrustedCert

    FakeHttpsServer server(otherTrustedCert, loadKey(QStringLiteral("othertrusted.key")));
    QVERIFY2(server.isListening(), qPrintable(server.errorString()));
    writeFile(dir.filePath(QStringLiteral("sunshine.conf")), QByteArrayLiteral("port = ") + QByteArray::number(server.port() - 1) + "\n");

    SunshineController controller(std::make_unique<FakeSunshineProcess>(),
                                   std::make_unique<FakeSunshineNetworkProbe>(),
                                   std::make_unique<QNetworkAccessManager>(),
                                   configDir,
                                   dir.filePath(QStringLiteral("start.lock")),
                                   [] {
                                       return QStringLiteral("/usr/bin/sunshine-fake");
                                   });

    QSignalSpy succeededSpy(&controller, &SunshineController::pairingSucceeded);
    QSignalSpy failedSpy(&controller, &SunshineController::pairingFailed);

    controller.pair(QStringLiteral("1234"), QStringLiteral("Test Device"), QStringLiteral("moonbeam"), QStringLiteral("secret"));

    QVERIFY2(!server.serveOnePairingRequest(), "a certificate that is globally trusted but not the pinned one must still be rejected");
    QVERIFY(!failedSpy.isEmpty() || failedSpy.wait(2000));
    QCOMPARE(succeededSpy.count(), 0);
}

QTEST_GUILESS_MAIN(SunshineControllerTest)
#include "sunshinecontrollertest.moc"
