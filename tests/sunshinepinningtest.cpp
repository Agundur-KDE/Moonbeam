#include "sunshineidentity.h"

#include <QFile>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
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

/**
 * Minimal stand-in for a Sunshine web UI: a TLS listener presenting
 * whichever certificate/key the test hands it, mirroring exactly what
 * SunshineController::isSunshineAt() talks to. Drives the TLS handshake
 * itself (setSocketDescriptor + startServerEncryption) rather than going
 * through QSslServer's automatic pending-connection queue, whose
 * newConnection()/nextPendingConnection() pairing proved unreliable for a
 * single-shot, same-thread client+server test like this one.
 */
class FakeSunshineServer : public QObject
{
public:
    FakeSunshineServer(QSslCertificate cert, QSslKey key)
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

    // Accepts one TCP connection and promotes it to TLS as the server
    // side, blocking (via a nested event loop) until that handshake
    // finishes. Returns nullptr if no connection or handshake arrives
    // within the timeout.
    QSslSocket *acceptAndEncrypt()
    {
        if (!m_tcpServer.hasPendingConnections()) {
            QSignalSpy newConnectionSpy(&m_tcpServer, &QTcpServer::newConnection);
            if (!newConnectionSpy.wait(2000)) {
                return nullptr;
            }
        }

        QTcpSocket *raw = m_tcpServer.nextPendingConnection();
        if (!raw) {
            return nullptr;
        }

        // dup() the fd rather than handing over raw's own descriptor: raw
        // still owns and will close its original on destruction, which
        // would otherwise pull the shared fd out from under the new
        // QSslSocket wrapping it.
        auto *tls = new QSslSocket(this);
        tls->setLocalCertificate(m_cert);
        tls->setPrivateKey(m_key);
        tls->setSocketDescriptor(::dup(raw->socketDescriptor()), QAbstractSocket::ConnectedState, QAbstractSocket::ReadWrite);
        raw->deleteLater();

        QSignalSpy encryptedSpy(tls, &QSslSocket::encrypted);
        tls->startServerEncryption();
        if (!tls->isEncrypted() && !encryptedSpy.wait(2000)) {
            return nullptr;
        }
        return tls;
    }

private:
    QTcpServer m_tcpServer;
    QSslCertificate m_cert;
    QSslKey m_key;
    bool m_listening = false;
};

// Waits for a client QSslSocket to finish an already-in-flight handshake.
// Plain waitForEncrypted() can spuriously report false when the handshake
// already completed while something else (acceptAndEncrypt() above) was
// pumping the same event loop, so isEncrypted() is checked first.
bool waitUntilEncrypted(QSslSocket &socket)
{
    return socket.isEncrypted() || socket.waitForEncrypted(2000);
}
}

class SunshinePinningTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void acceptsExactPinnedCertificate();
    void rejectsImposterWithSameCommonNameDifferentKey();
};

void SunshinePinningTest::acceptsExactPinnedCertificate()
{
    const QByteArray pinned = SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("real.pem")));
    QVERIFY(!pinned.isEmpty());

    FakeSunshineServer server(loadCert(QStringLiteral("real.pem")), loadKey(QStringLiteral("real.key")));
    QVERIFY2(server.isListening(), qPrintable(server.errorString()));

    QSslSocket client;
    client.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.connectToHostEncrypted(QStringLiteral("127.0.0.1"), server.port());

    QSslSocket *serverSide = server.acceptAndEncrypt();
    QVERIFY(serverSide);
    QVERIFY2(waitUntilEncrypted(client), qPrintable(client.errorString()));

    QVERIFY(SunshineIdentity::matchesPinnedFingerprint(client.peerCertificate(), pinned));
    client.abort();
}

void SunshinePinningTest::rejectsImposterWithSameCommonNameDifferentKey()
{
    // The S-01 exploit shape: the impostor's certificate has the exact same
    // CN ("Sunshine Gamestream Host") as the real one - a CN check would
    // have accepted it. Fingerprint pinning against the real cert must
    // still reject it.
    const QSslCertificate imposterCert = loadCert(QStringLiteral("imposter.pem"));
    const QByteArray pinnedToReal = SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("real.pem")));
    QVERIFY(!pinnedToReal.isEmpty());
    QCOMPARE(imposterCert.subjectInfo(QSslCertificate::CommonName), loadCert(QStringLiteral("real.pem")).subjectInfo(QSslCertificate::CommonName));

    FakeSunshineServer server(imposterCert, loadKey(QStringLiteral("imposter.key")));
    QVERIFY2(server.isListening(), qPrintable(server.errorString()));

    QSslSocket client;
    client.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.connectToHostEncrypted(QStringLiteral("127.0.0.1"), server.port());

    QSslSocket *serverSide = server.acceptAndEncrypt();
    QVERIFY(serverSide);
    QVERIFY2(waitUntilEncrypted(client), qPrintable(client.errorString()));

    QVERIFY(!SunshineIdentity::matchesPinnedFingerprint(client.peerCertificate(), pinnedToReal));
    client.abort();
}

QTEST_GUILESS_MAIN(SunshinePinningTest)
#include "sunshinepinningtest.moc"
