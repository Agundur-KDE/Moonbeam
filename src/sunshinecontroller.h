#pragma once

#include "isunshinenetworkprobe.h"
#include "isunshineprocess.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>
#include <memory>
#include <optional>
#include <qqmlintegration.h>

/**
 * Controller around a Sunshine process. Exposed to QML as a singleton
 * (QML_SINGLETON) rather than a per-page instance: it owns a live process
 * handle, and an earlier per-page instantiation meant navigating between
 * pages could destroy that member and silently kill a running Sunshine
 * that this controller had started - the singleton removes that lifecycle
 * hazard entirely.
 *
 * Never assumes it is the only thing that might start Sunshine: on every
 * refresh() it checks whether Sunshine's web UI port is already reachable
 * (an existing instance, ours or the system's) before deciding whether to
 * spawn a new one. stop() only ever terminates a process this controller
 * itself started (RunningOwned) - an externally running instance
 * (RunningExternal) is left alone.
 *
 * The web UI port is not hardcoded: Sunshine derives it as
 * (configured `port`, default 47989) + 1. refresh()/start() read that from
 * Sunshine's own config file when present, so a non-default port doesn't
 * cause a second instance to be started against the default port instead.
 *
 * A port merely responding is not proof it's Sunshine - some other local
 * process could be listening there. Before treating a port as an existing
 * Sunshine instance (and, critically, before ever sending pairing
 * credentials to it), the exact certificate fingerprint Sunshine is
 * configured to serve is pinned and checked (see SunshineIdentity).
 *
 * Known limitation: start() takes a QLockFile to close the most obvious
 * TOCTOU window (two Moonbeam processes racing to start Sunshine
 * simultaneously), but this is cooperative locking between Moonbeam
 * instances only - it does not protect against Sunshine being started by
 * some entirely different tool at the same moment.
 *
 * A periodic timer calls refresh() every few seconds so the UI notices
 * when a RunningExternal instance (one Moonbeam didn't start, so it isn't
 * told about via process signals) disappears on its own - without this,
 * the status would stay stuck on "Sharing desktop" after an external
 * Sunshine crashed or was stopped by something else.
 *
 * All real I/O (the Sunshine process, port/TLS probing, executable lookup,
 * config directory, and the start-lock path) is reached only through
 * injected seams (audit.txt R-01) - the public, QML-visible constructor
 * wires up the real ones; a second constructor exists purely for tests to
 * substitute fakes, so process crashes, timeouts, spoofed certificates, and
 * lock contention can be exercised deterministically without touching a
 * real process, socket, or the filesystem.
 */
class SunshineController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class State {
        Checking,
        NotInstalled,
        ConfigInvalid,
        Stopped,
        RunningExternal,
        RunningOwned,
    };
    Q_ENUM(State)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
    Q_PROPERTY(bool canStop READ canStop NOTIFY stateChanged)
    Q_PROPERTY(bool pairingInProgress READ pairingInProgress NOTIFY pairingInProgressChanged)
    Q_PROPERTY(bool viewOnly READ viewOnly WRITE setViewOnly NOTIFY viewOnlyChanged)

    using ExecutableFinder = std::function<QString()>;

    explicit SunshineController(QObject *parent = nullptr);

    /**
     * Test-only constructor: every real I/O seam is injected explicitly
     * instead of defaulting to the real filesystem/process/network.
     * configDir replaces "~/.config/sunshine" and startLockPath replaces
     * the real temp-dir lock file path, so tests never touch the real
     * Sunshine config or contend with a real Moonbeam instance's lock.
     */
    SunshineController(std::unique_ptr<ISunshineProcess> process,
                        std::unique_ptr<ISunshineNetworkProbe> networkProbe,
                        std::unique_ptr<QNetworkAccessManager> network,
                        QString configDir,
                        QString startLockPath,
                        ExecutableFinder executableFinder,
                        int startLockTimeoutMs = 2000,
                        QObject *parent = nullptr);

    State state() const;
    QString statusText() const;
    bool canStart() const;
    bool canStop() const;
    bool pairingInProgress() const;

    /**
     * Whether a paired Moonlight client only sees the screen, without
     * mouse/keyboard/controller control (audit.txt S-03: Sunshine's own
     * default is full control of every input device). Reflects
     * sunshine.conf on disk; only takes effect the next time Sunshine
     * starts, so the setter is a no-op unless canStart() is true.
     */
    bool viewOnly() const;
    void setViewOnly(bool viewOnly);

public Q_SLOTS:
    void refresh();
    void start();
    void stop();

    /**
     * Submit a Moonlight pairing PIN to Sunshine's local web UI API
     * (POST /api/pin on 127.0.0.1). webUiUser/webUiPassword should come
     * from the SunshineCredentials singleton (KWallet-backed) - QML reads
     * them from there once SunshineCredentials.state is Ready, so a human
     * never types or sees them in the common case. Kept as parameters
     * here (rather than this class owning a SunshineCredentials instance
     * itself) to avoid two independent instances both racing to generate
     * fresh credentials the first time a wallet entry doesn't exist yet.
     */
    void pair(const QString &pin, const QString &deviceName, const QString &webUiUser, const QString &webUiPassword);

Q_SIGNALS:
    void stateChanged();
    void viewOnlyChanged();
    void pairingInProgressChanged();
    void pairingSucceeded();
    void pairingFailed(const QString &reason);

private:
    void setState(State newState);
    bool isSunshineAt(quint16 port, int timeoutMs = 500) const;
    std::optional<quint16> resolveWebUiPort() const;
    QString sunshineConfigDir() const;
    QString sunshineConfigContents() const;
    bool writeSunshineConfigContents(const QString &contents) const;
    QByteArray pinnedCertificateFingerprint() const;

    std::unique_ptr<ISunshineProcess> m_process;
    std::unique_ptr<ISunshineNetworkProbe> m_networkProbe;
    std::unique_ptr<QNetworkAccessManager> m_network;
    QString m_configDir;
    QString m_startLockPath;
    int m_startLockTimeoutMs;
    ExecutableFinder m_executableFinder;
    QTimer m_refreshTimer;
    State m_state = State::Checking;
    bool m_pairingInProgress = false;
    QString m_executablePath;
};
