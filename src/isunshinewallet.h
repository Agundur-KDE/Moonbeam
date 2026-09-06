#pragma once

#include <QString>

/**
 * Seam around KWallet so tests can simulate a wallet that's locked,
 * unavailable, or fails to write, without touching a real KWallet daemon
 * (audit.txt R-01).
 */
class ISunshineWallet
{
public:
    virtual ~ISunshineWallet() = default;

    // Fills user/password and returns true iff both were found and
    // non-empty. Returns false (leaving user/password untouched) if the
    // wallet is unavailable, has no matching entry, or the entry is
    // incomplete.
    virtual bool readCredentials(QString &user, QString &password) const = 0;

    // Returns true iff the credentials were persisted successfully.
    virtual bool writeCredentials(const QString &user, const QString &password) = 0;
};

/** Real implementation: KWallet::Wallet's local wallet, "Moonbeam" folder. */
class KWalletSunshineWallet : public ISunshineWallet
{
public:
    bool readCredentials(QString &user, QString &password) const override;
    bool writeCredentials(const QString &user, const QString &password) override;
};
