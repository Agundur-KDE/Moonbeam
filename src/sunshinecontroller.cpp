#include "sunshinecontroller.h"
#include "sunshineaudioconfig.h"
#include "sunshineidentity.h"
#include "sunshinepairingresponse.h"
#include "sunshineportconfig.h"
#include "sunshineremotecontrolconfig.h"

#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStandardPaths>
#include <QUrl>

namespace
{
QString defaultConfigDir()
{
    return QDir::homePath() + QStringLiteral("/.config/sunshine");
}

QString defaultStartLockPath()
{
    return QDir::temp().filePath(QStringLiteral("moonbeam-sunshine-start.lock"));
}

QString findSunshineExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("sunshine"));
}
}

SunshineController::SunshineController(QObject *parent)
    : SunshineController(std::make_unique<QtSunshineProcess>(),
                          std::make_unique<QtSunshineNetworkProbe>(),
                          std::make_unique<QNetworkAccessManager>(),
                          defaultConfigDir(),
                          defaultStartLockPath(),
                          &findSunshineExecutable,
                          2000,
                          parent)
{
}

SunshineController::SunshineController(std::unique_ptr<ISunshineProcess> process,
                                        std::unique_ptr<ISunshineNetworkProbe> networkProbe,
                                        std::unique_ptr<QNetworkAccessManager> network,
                                        QString configDir,
                                        QString startLockPath,
                                        ExecutableFinder executableFinder,
                                        int startLockTimeoutMs,
                                        QObject *parent)
    : QObject(parent)
    , m_process(std::move(process))
    , m_networkProbe(std::move(networkProbe))
    , m_network(std::move(network))
    , m_configDir(std::move(configDir))
    , m_startLockPath(std::move(startLockPath))
    , m_startLockTimeoutMs(startLockTimeoutMs)
    , m_executableFinder(std::move(executableFinder))
{
    m_network->setParent(this);

    connect(m_process.get(), &ISunshineProcess::started, this, &SunshineController::refresh);
    connect(m_process.get(), &ISunshineProcess::finished, this, &SunshineController::refresh);
    connect(m_process.get(), &ISunshineProcess::errorOccurred, this, &SunshineController::refresh);

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
        return i18n("Checking Sunshine…");
    case State::NotInstalled:
        return i18n("Sunshine not found — install it first");
    case State::ConfigInvalid:
        return i18n("Sunshine's configured port is invalid — check sunshine.conf");
    case State::Stopped:
        return i18n("Not sharing");
    case State::RunningExternal:
        return i18n("Sharing desktop (using an already-running Sunshine)");
    case State::RunningOwned:
        return i18n("Sharing desktop");
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
    return m_configDir;
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

bool SunshineController::surroundAudio() const
{
    return SunshineAudioConfig::isSurroundEnabled(sunshineConfigContents());
}

void SunshineController::setSurroundAudio(bool surroundAudio)
{
    // Same start-only restriction as setViewOnly(), for the same reason:
    // Sunshine only reads audio_sink at process start.
    if (!canStart() || this->surroundAudio() == surroundAudio) {
        return;
    }

    if (!writeSunshineConfigContents(SunshineAudioConfig::withSurroundEnabled(sunshineConfigContents(), surroundAudio))) {
        return;
    }

    Q_EMIT surroundAudioChanged();
}

QString SunshineController::webUiUrl() const
{
    const auto port = resolveWebUiPort();
    if (!port.has_value()) {
        return {};
    }
    return QStringLiteral("https://127.0.0.1:%1").arg(*port);
}

QByteArray SunshineController::pinnedCertificateFingerprint() const
{
    const QString certPath = SunshineIdentity::certificatePath(sunshineConfigDir(), sunshineConfigContents());
    return SunshineIdentity::certificateFingerprint(certPath);
}

std::optional<quint16> SunshineController::resolveWebUiPort() const
{
    return SunshinePortConfig::resolveWebUiPort(sunshineConfigContents());
}

void SunshineController::refresh()
{
    if (m_process->state() != QProcess::NotRunning) {
        setState(State::RunningOwned);
        return;
    }

    const auto webUiPort = resolveWebUiPort();
    if (!webUiPort.has_value()) {
        // audit.txt S-06: an unparsable or out-of-range `port` in
        // sunshine.conf must never be guessed at (silently truncated, or
        // wrapped via overflow) - refuse to start, and say why.
        setState(State::ConfigInvalid);
        return;
    }

    if (m_networkProbe->isPortOpen(*webUiPort, 300) && isSunshineAt(*webUiPort)) {
        setState(State::RunningExternal);
        return;
    }

    if (m_executablePath.isEmpty()) {
        m_executablePath = m_executableFinder();
    }

    setState(m_executablePath.isEmpty() ? State::NotInstalled : State::Stopped);
}

void SunshineController::start()
{
    // Closes the TOCTOU window between the port check and spawning a
    // process: two Moonbeam processes racing here will have one win the
    // lock and the other block briefly on tryLock(), then see
    // RunningExternal on its own subsequent refresh() instead of also
    // spawning a process.
    QLockFile lockFile(m_startLockPath);
    if (!lockFile.tryLock(m_startLockTimeoutMs)) {
        refresh();
        return;
    }

    refresh();

    if (m_state != State::Stopped) {
        // Already running (ours or external), or not installed - nothing to do.
        return;
    }

    m_process->start(m_executablePath, {});
    m_process->waitForStarted(2000);
}

void SunshineController::stop()
{
    if (!canStop()) {
        return;
    }

    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
    }
}

void SunshineController::pair(const QString &pin, const QString &deviceName, const QString &webUiUser, const QString &webUiPassword)
{
    if (m_pairingInProgress) {
        return;
    }

    const auto webUiPort = resolveWebUiPort();
    if (!webUiPort.has_value()) {
        Q_EMIT pairingFailed(i18n("Sunshine's configured port is invalid - check sunshine.conf"));
        return;
    }

    const QByteArray pinnedFingerprint = pinnedCertificateFingerprint();
    if (pinnedFingerprint.isEmpty()) {
        Q_EMIT pairingFailed(i18n("Could not determine Sunshine's certificate - refusing to send credentials"));
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://127.0.0.1:%1/api/pin").arg(*webUiPort)));
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

    QNetworkReply *reply = m_network->post(request, QJsonDocument(body).toJson());
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

        switch (SunshinePairingResponse::parse(reply->readAll())) {
        case SunshinePairingResponse::Result::Success:
            Q_EMIT pairingSucceeded();
            break;
        case SunshinePairingResponse::Result::Rejected:
            Q_EMIT pairingFailed(i18n("Sunshine rejected the PIN"));
            break;
        case SunshinePairingResponse::Result::Malformed:
            Q_EMIT pairingFailed(i18n("Unexpected response from Sunshine"));
            break;
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

    const QSslCertificate cert = m_networkProbe->peerCertificateAt(port, timeoutMs);
    if (cert.isNull()) {
        return false;
    }

    return SunshineIdentity::matchesPinnedFingerprint(cert, pinnedFingerprint);
}
