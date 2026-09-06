#pragma once

#include "isunshinewallet.h"

/**
 * Test double for ISunshineWallet: no real KWallet daemon is ever touched.
 * Tests configure whether credentials are already present and whether
 * reads/writes succeed, to simulate a locked/unavailable/failing wallet.
 */
class FakeSunshineWallet : public ISunshineWallet
{
public:
    bool hasCredentials = false;
    QString storedUser;
    QString storedPassword;
    bool readShouldSucceed = true;
    bool writeShouldSucceed = true;

    int writeCallCount = 0;

    bool readCredentials(QString &user, QString &password) const override
    {
        if (!readShouldSucceed || !hasCredentials) {
            return false;
        }
        user = storedUser;
        password = storedPassword;
        return true;
    }

    bool writeCredentials(const QString &user, const QString &password) override
    {
        writeCallCount++;
        if (!writeShouldSucceed) {
            return false;
        }
        hasCredentials = true;
        storedUser = user;
        storedPassword = password;
        return true;
    }
};
