#include "sunshinecontroller.h"

SunshineController::SunshineController(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::stateChanged, this, [this] {
        Q_EMIT runningChanged();
    });
}

bool SunshineController::running() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString SunshineController::statusText() const
{
    return running() ? QStringLiteral("Sharing desktop") : QStringLiteral("Not sharing");
}

void SunshineController::start()
{
    // Sketch stage: assumes a system-wide `sunshine` binary and a pre-existing
    // config. Real version needs to locate/generate config, handle KWin
    // self-authorization (see kwingrab.cpp), and surface the pairing PIN.
    m_process.start(QStringLiteral("sunshine"), {});
}

void SunshineController::stop()
{
    m_process.terminate();
}
