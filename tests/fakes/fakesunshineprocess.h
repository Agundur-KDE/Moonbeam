#pragma once

#include "isunshineprocess.h"

#include <QStringList>

/**
 * Test double for ISunshineProcess: no real process is ever spawned. Tests
 * drive its state and observable calls directly, and can emit started()/
 * finished()/errorOccurred() manually to simulate a crash.
 */
class FakeSunshineProcess : public ISunshineProcess
{
    Q_OBJECT

public:
    void start(const QString &program, const QStringList &arguments) override
    {
        startCallCount++;
        lastProgram = program;
        lastArguments = arguments;
        if (startShouldSucceed) {
            m_state = QProcess::Running;
            Q_EMIT started();
        }
    }

    bool waitForStarted(int) override
    {
        return startShouldSucceed;
    }

    void terminate() override
    {
        terminateCallCount++;
    }

    void kill() override
    {
        killCallCount++;
        m_state = QProcess::NotRunning;
        Q_EMIT finished();
    }

    bool waitForFinished(int) override
    {
        if (finishesOnTerminate) {
            m_state = QProcess::NotRunning;
        }
        return finishesOnTerminate;
    }

    QProcess::ProcessState state() const override
    {
        return m_state;
    }

    // --- test control surface ---
    QProcess::ProcessState m_state = QProcess::NotRunning;
    bool startShouldSucceed = true;
    // Whether waitForFinished() (called by stop() after terminate())
    // reports success - false simulates a hung process that only responds
    // to kill().
    bool finishesOnTerminate = true;

    int startCallCount = 0;
    int terminateCallCount = 0;
    int killCallCount = 0;
    QString lastProgram;
    QStringList lastArguments;
};
