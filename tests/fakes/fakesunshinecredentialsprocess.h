#pragma once

#include "isunshinecredentialsprocess.h"

/**
 * Test double for ISunshineCredentialsProcess: no real `sunshine --creds`
 * process is ever spawned. Tests configure whether it "succeeds" and can
 * inspect what it was called with.
 */
class FakeSunshineCredentialsProcess : public ISunshineCredentialsProcess
{
public:
    bool shouldSucceed = true;
    int callCount = 0;
    QString lastUser;
    QString lastPassword;

    bool setCredentials(const QString &, const QString &user, const QString &password, int) override
    {
        callCount++;
        lastUser = user;
        lastPassword = password;
        return shouldSucceed;
    }
};
