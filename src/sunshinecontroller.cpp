#include "sunshinecontroller.h"
#include "sunshineidentity.h"
#include "sunshineremotecontrolconfig.h"

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

QString SunshineController::sunshineConfigDir() const
{
    return QDir::homePath() + QStringLiteral("/.config/sunshine");
}

QString SunshineController::sunshineConfigContents() const
{
    QFile file(sunshineConfigDir() + QStringLiteral("/sunshine.conf"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool SunshineController::writeSunshineConfigContents(const QString &contents) const
{
    QDir().mkpath(sunshineConfigDir());
    QFile file(sunshineConfigDir() + QStringLiteral("/sunshine.conf"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    return file.write(contents.toUtf8()) >= 0;
}

bool SunshineController::viewOnly() const
{
    return SunshineRemoteControlConfig::isViewOnly(sunshineConfigContents());
}

void SunshineController::setViewOnly(bool viewOnly)
{
    // Sunshine only reads its config at process start, so toggling this
    // while a "RunningOwned"/"RunningExternal" instance is already up
    // would silently do nothing until the next restart - only allow it
    // while stopped, so the property always reflects what will actually
    // apply.
    if (!canStart() || this->viewOnly() == viewOnly) {
        return;
    }

    if (!writeSunshineConfigContents(SunshineRemoteControlConfig::withViewOnly(sunshineConfigContents(), viewOnly))) {
        return;
    }

    Q_EMIT viewOnlyChanged();
}

QByteArray SunshineController::pinnedCertificateFingerprint() const
{
    const QString certPath = SunshineIdentity::certificatePath(sunshineConfigDir(), sunshineConfigContents());
    return SunshineIdentity::certificateFingerprint(certPath);
}

quint16 SunshineController::resolveWebUiPort() const
{
    quint16 basePort = defaultBasePort();

    static const QRegularExpression portLine(QStringLiteral("^\\s*port\\s*=\\s*(\\d+)\\s*$"), QRegularExpression::MultilineOption);
    auto it = portLine.globalMatch(sunshineConfigContents());
    while (it.hasNext()) {
        basePort = static_cast<quint16>(it.next().captured(1).toUInt());
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

    const QByteArray pinnedFingerprint = pinnedCertificateFingerprint();
    if (pinnedFingerprint.isEmpty()) {
        Q_EMIT pairingFailed(QStringLiteral("Could not determine Sunshine's certificate - refusing to send credentials"));
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://127.0.0.1:%1/api/pin").arg(webUiPort)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QByteArray credentials = (webUiUser + QStringLiteral(":") + webUiPassword).toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + credentials);

    // Sunshine's cert is self-signed, so the CA-chain check below is
    // expected to fail - that alone is not a reason to reject it. What
    // actually decides trust is the sslErrors handler below, which pins
    // the exact certificate fingerprint (see SunshineIdentity) and only
    // proceeds - meaning only then does the Authorization header above
    // actually get sent - when that fingerprint matches. This also closes
    // the TOCTOU a separate isSunshineAt() probe-then-connect would leave
    // open: verification happens on this exact TLS connection, not a
    // previous one that something could have swapped out afterward.
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
    connect(reply, &QNetworkReply::sslErrors, reply, [reply, pinnedFingerprint](const QList<QSslError> &) {
        if (SunshineIdentity::matchesPinnedFingerprint(reply->sslConfiguration().peerCertificate(), pinnedFingerprint)) {
            reply->ignoreSslErrors();
        } else {
            reply->abort();
        }
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
    // Pins the exact certificate Sunshine is configured to serve (read from
    // its own config, see SunshineIdentity) rather than checking the
    // certificate's CN: a CN is just a string inside a self-signed
    // certificate, and any local process can put the same one in a
    // certificate of its own (see audit.txt S-01). An unresolvable pin
    // (config/cert unreadable) means "cannot verify" and fails closed.
    const QByteArray pinnedFingerprint = pinnedCertificateFingerprint();
    if (pinnedFingerprint.isEmpty()) {
        return false;
    }

    QSslSocket socket;
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    socket.connectToHostEncrypted(QStringLiteral("127.0.0.1"), port);
    if (!socket.waitForEncrypted(timeoutMs)) {
        return false;
    }

    const QSslCertificate cert = socket.peerCertificate();
    socket.abort();
    return SunshineIdentity::matchesPinnedFingerprint(cert, pinnedFingerprint);
}
