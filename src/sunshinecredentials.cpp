// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "sunshinecredentials.h"
#include "sunshinecredentialsstate.h"

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QLockFile>
#include <QRandomGenerator>
#include <QStandardPaths>

namespace
{
const QString GeneratedUser = QStringLiteral("moonbeam");

QString defaultStateFilePath()
{
    return QDir::homePath() + QStringLiteral("/.config/sunshine/sunshine_state.json");
}

// Serializes the whole "figure out/set up Sunshine web UI credentials"
// sequence across Moonbeam processes (audit.txt S-02): without this, two
// processes could each decide independently that no credentials exist yet,
// generate different passwords, both run `sunshine --creds`, and end up
// with Sunshine's actual password and KWallet's stored copy disagreeing.
QString defaultLockPath()
{
    return QDir::temp().filePath(QStringLiteral("moonbeam-sunshine-credentials.lock"));
}

QString findSunshineExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("sunshine"));
}

QString generateRandomPassword()
{
    static const QString alphabet = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789");
    QString password;
    for (int i = 0; i < 24; ++i) {
        password.append(alphabet.at(QRandomGenerator::global()->bounded(alphabet.size())));
    }
    return password;
}
}

SunshineCredentials::SunshineCredentials(QObject *parent)
    : SunshineCredentials(std::make_unique<KWalletSunshineWallet>(),
                           std::make_unique<QtSunshineCredentialsProcess>(),
                           defaultStateFilePath(),
                           defaultLockPath(),
                           &findSunshineExecutable,
                           &generateRandomPassword,
                           5000,
                           parent)
{
}

SunshineCredentials::SunshineCredentials(std::unique_ptr<ISunshineWallet> wallet,
                                          std::unique_ptr<ISunshineCredentialsProcess> credentialsProcess,
                                          QString stateFilePath,
                                          QString lockPath,
                                          ExecutableFinder executableFinder,
                                          PasswordGenerator passwordGenerator,
                                          int lockTimeoutMs,
                                          QObject *parent)
    : QObject(parent)
    , m_wallet(std::move(wallet))
    , m_credentialsProcess(std::move(credentialsProcess))
    , m_stateFilePath(std::move(stateFilePath))
    , m_lockPath(std::move(lockPath))
    , m_lockTimeoutMs(lockTimeoutMs)
    , m_executableFinder(std::move(executableFinder))
    , m_passwordGenerator(std::move(passwordGenerator))
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

    QLockFile lockFile(m_lockPath);
    if (!lockFile.tryLock(m_lockTimeoutMs)) {
        setState(State::Failed);
        return;
    }

    // Re-check after acquiring the lock: another Moonbeam process may have
    // already finished this exact setup while we were waiting for it.
    QString walletUser;
    QString walletPassword;
    if (m_wallet->readCredentials(walletUser, walletPassword)) {
        m_user = walletUser;
        m_password = walletPassword;
        setState(State::Ready);
        return;
    }

    const auto probe = SunshineCredentialsState::probeConfiguredUsername(m_stateFilePath);
    if (probe.result == SunshineCredentialsState::Result::Indeterminate) {
        // Could not positively identify a fresh install (unreadable file,
        // unrecognized format, ...) - never guess "empty" from a read or
        // parse failure, since that risks overwriting a real password.
        setState(State::Failed);
        return;
    }

    if (probe.result == SunshineCredentialsState::Result::ExistingUser) {
        // Sunshine already has credentials we don't know - set by a human,
        // or an earlier non-Moonbeam setup. Never guess or overwrite these.
        m_user = probe.username;
        setState(State::NeedsExistingPassword);
        return;
    }

    // Positively identified as fresh: no web UI credentials configured yet,
    // safe to generate our own and set them via `sunshine --creds`, which
    // works standalone without Sunshine needing to already be running.
    const QString sunshineBin = m_executableFinder();
    if (sunshineBin.isEmpty()) {
        setState(State::Failed);
        return;
    }

    const QString user = GeneratedUser;
    const QString password = m_passwordGenerator();

    // audit.txt S-04: Sunshine's own CLI (`sunshine --help`) offers no way
    // to set web UI credentials other than as plain `--creds user pass`
    // arguments - no stdin or protected-file option exists to set them via.
    // That leaves a brief window (until this process exits, a few seconds
    // at most) where another process belonging to this user, or a process
    // monitor, could read the password from /proc. Not eliminable without
    // a change on Sunshine's side; kept as small as possible by not
    // logging the command line and letting the process exit immediately
    // after this call.
    if (!m_credentialsProcess->setCredentials(sunshineBin, user, password, 5000)) {
        setState(State::Failed);
        return;
    }

    if (!m_wallet->writeCredentials(user, password)) {
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

    QLockFile lockFile(m_lockPath);
    if (!lockFile.tryLock(m_lockTimeoutMs)) {
        setState(State::Failed);
        return;
    }

    if (!m_wallet->writeCredentials(m_user, password)) {
        setState(State::Failed);
        return;
    }

    m_password = password;
    setState(State::Ready);
}

void SunshineCredentials::copyPasswordToClipboard() const
{
    QGuiApplication::clipboard()->setText(m_password);
}

void SunshineCredentials::setState(State newState)
{
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    Q_EMIT stateChanged();
}
