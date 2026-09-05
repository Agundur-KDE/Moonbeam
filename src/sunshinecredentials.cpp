#include "sunshinecredentials.h"

#include <KWallet>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>

namespace
{
const QString WalletFolder = QStringLiteral("Moonbeam");
const QString WalletKey = QStringLiteral("sunshine-webui");
const QString GeneratedUser = QStringLiteral("moonbeam");
}

SunshineCredentials::SunshineCredentials(QObject *parent)
    : QObject(parent)
{
}

SunshineCredentials::State SunshineCredentials::state() const
{
    return m_state;
}

QString SunshineCredentials::user() const
{
    return m_user;
}

QString SunshineCredentials::password() const
{
    return m_password;
}

void SunshineCredentials::ensure()
{
    if (m_state == State::Ready) {
        return;
    }

    if (loadFromWallet()) {
        setState(State::Ready);
        return;
    }

    const QString existingUser = readConfiguredUsername();
    if (!existingUser.isEmpty()) {
        // Sunshine already has credentials we don't know - set by a human,
        // or an earlier non-Moonbeam setup. Never guess or overwrite these.
        m_user = existingUser;
        setState(State::NeedsExistingPassword);
        return;
    }

    // Fresh Sunshine with no web UI credentials configured yet - safe to
    // generate our own and set them via `sunshine --creds`, which works
    // standalone without Sunshine needing to already be running.
    const QString sunshineBin = findSunshineExecutable();
    if (sunshineBin.isEmpty()) {
        setState(State::Failed);
        return;
    }

    const QString user = GeneratedUser;
    const QString password = generateRandomPassword();

    QProcess creds;
    creds.start(sunshineBin, {QStringLiteral("--creds"), user, password});
    creds.waitForFinished(5000);

    if (creds.exitStatus() != QProcess::NormalExit || creds.exitCode() != 0) {
        setState(State::Failed);
        return;
    }

    if (!saveToWallet(user, password)) {
        setState(State::Failed);
        return;
    }

    m_user = user;
    m_password = password;
    setState(State::Ready);
}

void SunshineCredentials::provideExisting(const QString &password)
{
    if (m_state != State::NeedsExistingPassword || m_user.isEmpty()) {
        return;
    }

    if (!saveToWallet(m_user, password)) {
        setState(State::Failed);
        return;
    }

    m_password = password;
    setState(State::Ready);
}

void SunshineCredentials::setState(State newState)
{
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    Q_EMIT stateChanged();
}

bool SunshineCredentials::loadFromWallet()
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

    m_user = map.value(QStringLiteral("user"));
    m_password = map.value(QStringLiteral("password"));
    return !m_user.isEmpty() && !m_password.isEmpty();
}

bool SunshineCredentials::saveToWallet(const QString &user, const QString &password)
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

QString SunshineCredentials::readConfiguredUsername() const
{
    QFile file(QDir::homePath() + QStringLiteral("/.config/sunshine/sunshine_state.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }

    return document.object().value(QStringLiteral("username")).toString();
}

QString SunshineCredentials::findSunshineExecutable() const
{
    return QStandardPaths::findExecutable(QStringLiteral("sunshine"));
}

QString SunshineCredentials::generateRandomPassword() const
{
    static const QString alphabet = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789");
    QString password;
    for (int i = 0; i < 24; ++i) {
        password.append(alphabet.at(QRandomGenerator::global()->bounded(alphabet.size())));
    }
    return password;
}
