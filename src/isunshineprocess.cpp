#include "isunshineprocess.h"

QtSunshineProcess::QtSunshineProcess(QObject *parent)
    : ISunshineProcess(parent)
{
    connect(&m_process, &QProcess::started, this, &ISunshineProcess::started);
    connect(&m_process, &QProcess::finished, this, &ISunshineProcess::finished);
    connect(&m_process, &QProcess::errorOccurred, this, &ISunshineProcess::errorOccurred);
}

void QtSunshineProcess::start(const QString &program, const QStringList &arguments)
{
    m_process.start(program, arguments);
}

bool QtSunshineProcess::waitForStarted(int timeoutMs)
{
    return m_process.waitForStarted(timeoutMs);
}

void QtSunshineProcess::terminate()
{
    m_process.terminate();
}

void QtSunshineProcess::kill()
{
    m_process.kill();
}

bool QtSunshineProcess::waitForFinished(int timeoutMs)
{
    return m_process.waitForFinished(timeoutMs);
}

QProcess::ProcessState QtSunshineProcess::state() const
{
    return m_process.state();
}
