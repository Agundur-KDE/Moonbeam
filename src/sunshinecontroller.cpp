#include "sunshinecontroller.h"

#include <QStandardPaths>
#include <QTcpSocket>

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
