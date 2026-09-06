#pragma once

#include <QString>

/**
 * Seam around the one-shot `sunshine --creds user password` invocation so
 * tests can simulate it succeeding, failing, or hanging without spawning a
 * real process (audit.txt R-01).
 */
class ISunshineCredentialsProcess
{
public:
    virtual ~ISunshineCredentialsProcess() = default;

    // Runs the equivalent of `sunshineBin --creds user password`, waiting
    // up to timeoutMs. Returns true iff it exited normally with code 0.
    virtual bool setCredentials(const QString &sunshineBin, const QString &user, const QString &password, int timeoutMs) = 0;
};

/** Real implementation: a synchronous QProcess run. */
class QtSunshineCredentialsProcess : public ISunshineCredentialsProcess
{
public:
    bool setCredentials(const QString &sunshineBin, const QString &user, const QString &password, int timeoutMs) override;
};
