#include "isunshinewallet.h"

#include <KWallet>

#include <QMap>

#include <memory>

namespace
{
const QString WalletFolder = QStringLiteral("Moonbeam");
const QString WalletKey = QStringLiteral("sunshine-webui");
}

bool KWalletSunshineWallet::readCredentials(QString &user, QString &password) const
{
    std::unique_ptr<KWallet::Wallet> wallet(
        KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0, KWallet::Wallet::Synchronous));
    if (!wallet || !wallet->hasFolder(WalletFolder)) {
        return false;
    }

    wallet->setFolder(WalletFolder);
    QMap<QString, QString> map;
    if (wallet->readMap(WalletKey, map) != 0) {
        return false;
    }

    const QString foundUser = map.value(QStringLiteral("user"));
    const QString foundPassword = map.value(QStringLiteral("password"));
    if (foundUser.isEmpty() || foundPassword.isEmpty()) {
        return false;
    }

    user = foundUser;
    password = foundPassword;
    return true;
}

bool KWalletSunshineWallet::writeCredentials(const QString &user, const QString &password)
{
    std::unique_ptr<KWallet::Wallet> wallet(
        KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0, KWallet::Wallet::Synchronous));
    if (!wallet) {
        return false;
    }

    if (!wallet->hasFolder(WalletFolder) && !wallet->createFolder(WalletFolder)) {
        return false;
    }
    wallet->setFolder(WalletFolder);

    QMap<QString, QString> map;
    map[QStringLiteral("user")] = user;
    map[QStringLiteral("password")] = password;
    return wallet->writeMap(WalletKey, map) == 0;
}
