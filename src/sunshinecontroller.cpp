#include "sunshinecontroller.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>

SunshineController::SunshineController(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started, this, &SunshineController::refresh);
    connect(&m_process, &QProcess::finished, this, &SunshineController::refresh);
    connect(&m_process, &QProcess::errorOccurred, this, &SunshineController::refresh);

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

void SunshineController::refresh()
{
    if (m_process.state() != QProcess::NotRunning) {
        setState(State::RunningOwned);
        return;
    }

    if (isPortOpen(WebUiPort)) {
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
    refresh();

    if (m_state != State::Stopped) {
        // Already running (ours or external), or not installed - nothing to do.
        return;
    }

    m_process.start(m_executablePath, {});
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

    QNetworkRequest request(QUrl(QStringLiteral("https://127.0.0.1:%1/api/pin").arg(WebUiPort)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QByteArray credentials = (webUiUser + QStringLiteral(":") + webUiPassword).toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + credentials);

    // Sunshine's cert is self-signed; we're talking to our own local
    // instance over loopback (not a real network hop an attacker could
    // sit on without already having a foothold on this machine), so
    // there's no meaningful identity to verify. Deliberately not doing
    // certificate pinning here - real mitigation for a threat model this
    // app doesn't have.
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
