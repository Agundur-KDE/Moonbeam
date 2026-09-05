#pragma once

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

    explicit SunshineController(QObject *parent = nullptr);

    State state() const;
    QString statusText() const;
    bool canStart() const;
    bool canStop() const;

public Q_SLOTS:
    void refresh();
    void start();
    void stop();

Q_SIGNALS:
    void stateChanged();

private:
    void setState(State newState);
    bool isPortOpen(quint16 port, int timeoutMs = 300) const;

    QProcess m_process;
    State m_state = State::Checking;
    QString m_executablePath;

    static constexpr quint16 WebUiPort = 47990;
};
