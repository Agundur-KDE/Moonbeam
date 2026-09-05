#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QString>
#include <qqmlintegration.h>

/**
 * Controller around a Sunshine process.
 *
 * Never assumes it is the only thing that might start Sunshine: on every
 * refresh() it checks whether Sunshine's web UI port is already reachable
 * (an existing instance, ours or the system's) before deciding whether to
 * spawn a new one. stop() only ever terminates a process this controller
 * itself started (RunningOwned) - an externally running instance
 * (RunningExternal) is left alone.
 *
 * Known limitation: the port check is not a lock. There is a TOCTOU window
 * between refresh() and start() - two SunshineController instances (or two
 * Moonbeam processes) checking at the same moment could both see Stopped
 * and both start a process. Acceptable for a single-user desktop app driven
 * by manual button clicks; would need a real lock file for anything more
 * concurrent than that.
 *
 * Pairing/PIN handling and config generation are not implemented yet.
 */
class SunshineController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class State {
        Checking,
        NotInstalled,
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

    explicit SunshineController(QObject *parent = nullptr);

    State state() const;
    QString statusText() const;
    bool canStart() const;
    bool canStop() const;
    bool pairingInProgress() const;

public Q_SLOTS:
    void refresh();
    void start();
    void stop();

    /**
     * Submit a Moonlight pairing PIN to Sunshine's local web UI API
     * (POST /api/pin on 127.0.0.1). Requires the web UI's own username/
     * password - verified live that Sunshine does NOT exempt loopback
     * connections from authentication (a plausible reading of the source
     * turned out to be wrong for the installed version; confirmed with curl
     * before wiring this up).
     */
    void pair(const QString &pin, const QString &deviceName, const QString &webUiUser, const QString &webUiPassword);

Q_SIGNALS:
    void stateChanged();
    void pairingInProgressChanged();
    void pairingSucceeded();
    void pairingFailed(const QString &reason);

private:
    void setState(State newState);
    bool isPortOpen(quint16 port, int timeoutMs = 300) const;

    QProcess m_process;
    QNetworkAccessManager m_network;
    State m_state = State::Checking;
    bool m_pairingInProgress = false;
    QString m_executablePath;

    static constexpr quint16 WebUiPort = 47990;
};
