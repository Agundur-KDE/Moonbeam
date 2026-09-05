#include "sunshinecontroller.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>

namespace
{
// Sunshine's own server certificate CN, set in httpcommon.cpp
// (crypto::gen_creds("Sunshine Gamestream Host", ...)). Used to confirm
// that whatever is listening on the expected port is actually Sunshine,
// not some other local process that happens to have grabbed the port.
const QString SunshineCertCommonName = QStringLiteral("Sunshine Gamestream Host");

// Sunshine derives its web UI port as (configured base `port`, default
// 47989) + 1 (confighttp::PORT_HTTPS offset), see config.cpp. Reading the
// actual config avoids treating a custom-port Sunshine as "not running"
// and starting a conflicting second instance on the default port.
quint16 defaultBasePort()
{
    return 47989;
}
}

SunshineController::SunshineController(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started, this, &SunshineController::refresh);
    connect(&m_process, &QProcess::finished, this, &SunshineController::refresh);
    connect(&m_process, &QProcess::errorOccurred, this, &SunshineController::refresh);

    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &SunshineController::refresh);
    m_refreshTimer.start();

    refresh();
}

SunshineController::State SunshineController::state() const
{
    return m_state;
}

QString SunshineController::statusText() const
{
    switch (m_state) {
    case State::Checking:
        return QStringLiteral("Checking Sunshine…");
    case State::NotInstalled:
        return QStringLiteral("Sunshine not found — install it first");
    case State::Stopped:
        return QStringLiteral("Not sharing");
    case State::RunningExternal:
        return QStringLiteral("Sharing desktop (using an already-running Sunshine)");
    case State::RunningOwned:
        return QStringLiteral("Sharing desktop");
    }
    return {};
}

bool SunshineController::canStart() const
{
    return m_state == State::Stopped;
}

bool SunshineController::canStop() const
{
    // Deliberately excludes RunningExternal: never terminate a Sunshine
    // instance this controller did not start itself.
    return m_state == State::RunningOwned;
}

bool SunshineController::pairingInProgress() const
{
    return m_pairingInProgress;
}

quint16 SunshineController::resolveWebUiPort() const
{
    const QString configPath = QDir::homePath() + QStringLiteral("/.config/sunshine/sunshine.conf");
    QFile file(configPath);
    quint16 basePort = defaultBasePort();

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        static const QRegularExpression portLine(QStringLiteral("^\\s*port\\s*=\\s*(\\d+)\\s*$"));
        while (!file.atEnd()) {
            const auto match = portLine.match(QString::fromUtf8(file.readLine()));
            if (match.hasMatch()) {
                basePort = static_cast<quint16>(match.captured(1).toUInt());
            }
        }
    }

    return basePort + 1;
}

void SunshineController::refresh()
{
    if (m_process.state() != QProcess::NotRunning) {
        setState(State::RunningOwned);
        return;
    }

    const quint16 webUiPort = resolveWebUiPort();
    if (isPortOpen(webUiPort) && isSunshineAt(webUiPort)) {
        setState(State::RunningExternal);
        return;
    }

    if (m_executablePath.isEmpty()) {
        m_executablePath = QStandardPaths::findExecutable(QStringLiteral("sunshine"));
    }

    setState(m_executablePath.isEmpty() ? State::NotInstalled : State::Stopped);
}

void SunshineController::start()
{
    // Closes the TOCTOU window between the port check and QProcess::start():
    // two Moonbeam processes racing here will have one win the lock and the
    // other block briefly on tryLock(), then see RunningExternal on its own
    // subsequent refresh() instead of also spawning a process.
    QLockFile lockFile(QDir::temp().filePath(QStringLiteral("moonbeam-sunshine-start.lock")));
    if (!lockFile.tryLock(2000)) {
        refresh();
        return;
    }

    refresh();

    if (m_state != State::Stopped) {
        // Already running (ours or external), or not installed - nothing to do.
        return;
    }

    m_process.start(m_executablePath, {});
    m_process.waitForStarted(2000);
}

void SunshineController::stop()
{
    if (!canStop()) {
        return;
    }

    m_process.terminate();
    if (!m_process.waitForFinished(3000)) {
        m_process.kill();
    }
}

void SunshineController::pair(const QString &pin, const QString &deviceName, const QString &webUiUser, const QString &webUiPassword)
{
    if (m_pairingInProgress) {
        return;
    }

    const quint16 webUiPort = resolveWebUiPort();

    if (!isSunshineAt(webUiPort)) {
        Q_EMIT pairingFailed(QStringLiteral("Could not verify a Sunshine instance on port %1 - refusing to send credentials").arg(webUiPort));
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://127.0.0.1:%1/api/pin").arg(webUiPort)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QByteArray credentials = (webUiUser + QStringLiteral(":") + webUiPassword).toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + credentials);

    // Sunshine's cert is self-signed; we already verified its CN above, so
    // this isn't blind trust - just skipping the CA-chain check that a
    // self-signed cert could never pass anyway.
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
    request.setTransferTimeout(5000);

    const QJsonObject body {
        {QStringLiteral("pin"), pin},
        {QStringLiteral("name"), deviceName},
    };

    m_pairingInProgress = true;
    Q_EMIT pairingInProgressChanged();

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
        reply->ignoreSslErrors();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_pairingInProgress = false;
        Q_EMIT pairingInProgressChanged();

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT pairingFailed(reply->errorString());
            return;
        }

        const auto document = QJsonDocument::fromJson(reply->readAll());
        if (!document.isObject()) {
            Q_EMIT pairingFailed(QStringLiteral("Unexpected response from Sunshine"));
            return;
        }

        if (document.object().value(QStringLiteral("status")).toBool()) {
            Q_EMIT pairingSucceeded();
        } else {
            Q_EMIT pairingFailed(QStringLiteral("Sunshine rejected the PIN"));
        }
    });
}

void SunshineController::setState(State newState)
{
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    Q_EMIT stateChanged();
}

bool SunshineController::isPortOpen(quint16 port, int timeoutMs) const
{
    QTcpSocket socket;
    socket.connectToHost(QStringLiteral("127.0.0.1"), port);
    const bool connected = socket.waitForConnected(timeoutMs);
    socket.abort();
    return connected;
}

bool SunshineController::isSunshineAt(quint16 port, int timeoutMs) const
{
    QSslSocket socket;
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    socket.connectToHostEncrypted(QStringLiteral("127.0.0.1"), port);
    if (!socket.waitForEncrypted(timeoutMs)) {
        return false;
    }

    const QSslCertificate cert = socket.peerCertificate();
    socket.abort();
    return cert.subjectInfo(QSslCertificate::CommonName).contains(SunshineCertCommonName);
}
