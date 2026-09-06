#pragma once

#include "isunshinecredentialsprocess.h"
#include "isunshinewallet.h"

#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <qqmlintegration.h>

/**
 * Owns Moonbeam's relationship with Sunshine's web UI credentials, so a
 * real user is never asked to look up or type them in the common case.
 *
 * On first use against a Sunshine instance that has no web UI credentials
 * configured yet, this generates a random username/password, sets them via
 * `sunshine --creds <user> <pass>`, and stores them in KWallet. On every
 * later run, the stored credentials are read back from KWallet - no
 * dialog, no password to remember.
 *
 * If Sunshine already had credentials configured by someone else (or an
 * earlier, non-Moonbeam setup) before Moonbeam ever touched it, this is
 * detected by reading Sunshine's own state file rather than guessing (see
 * SunshineCredentialsState - a read/parse failure there is never treated as
 * "no credentials"), and the one-time existing password is asked for via
 * provideExisting() - after which it's stored in KWallet the same way and
 * never asked again.
 *
 * ensure() and provideExisting() both take a cross-process QLockFile before
 * touching the wallet, Sunshine's state, or spawning `sunshine --creds`,
 * and re-check the wallet immediately after acquiring it - otherwise two
 * Moonbeam processes could each decide independently that no credentials
 * exist yet and race to set different ones.
 *
 * The wallet and the `sunshine --creds` process are reached only through
 * injected seams (audit.txt R-01) - the public, QML-visible constructor
 * wires up the real ones (KWallet, a real QProcess); a second constructor
 * exists purely for tests to substitute fakes, so wallet failures, process
 * crashes/timeouts, and lock contention can be exercised deterministically
 * without touching a real wallet daemon, process, or the filesystem.
 */
class SunshineCredentials : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class State {
        Unknown,
        Ready,
        NeedsExistingPassword,
        Failed,
    };
    Q_ENUM(State)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString user READ user NOTIFY stateChanged)
    // audit.txt S-07: exposing the password to QML at all widens the
    // disclosure surface (any QML/JS code with access to this singleton
    // can read it, not just the one pairing call site that currently
    // does). Two known, deliberate readers as of this writing:
    // SunshineController::pair() needs it for HTTP Basic Auth (see that
    // function's own doc comment for why the lookup isn't done in C++
    // instead), and StatusPage.qml displays it (masked, with a copy
    // button) so the user can log into Sunshine's own web UI, which asks
    // for these same credentials via a browser Basic-Auth prompt - showing
    // the account owner their own password behind a reveal/copy action is
    // a different threat model than the "any QML code in the app can read
    // it" concern this note is about, so that binding is a deliberate
    // exception, not an oversight.
    Q_PROPERTY(QString password READ password NOTIFY stateChanged)

    using ExecutableFinder = std::function<QString()>;
    using PasswordGenerator = std::function<QString()>;

    explicit SunshineCredentials(QObject *parent = nullptr);

    /**
     * Test-only constructor: every real I/O seam is injected explicitly.
     * stateFilePath/lockPath replace the real "~/.config/sunshine/..." and
     * temp-dir paths, so tests never touch real files or contend with a
     * real Moonbeam instance's lock.
     */
    SunshineCredentials(std::unique_ptr<ISunshineWallet> wallet,
                         std::unique_ptr<ISunshineCredentialsProcess> credentialsProcess,
                         QString stateFilePath,
                         QString lockPath,
                         ExecutableFinder executableFinder,
                         PasswordGenerator passwordGenerator,
                         int lockTimeoutMs = 5000,
                         QObject *parent = nullptr);

    State state() const;
    QString user() const;
    QString password() const;

public Q_SLOTS:
    /**
     * Makes sure credentials are ready to use, generating and storing new
     * ones if Sunshine has none configured yet. Safe to call repeatedly.
     */
    void ensure();

    /**
     * Only relevant when state() is NeedsExistingPassword: provide the
     * password that was already set (by a human, or an earlier setup)
     * before Moonbeam ever ran. Stored in KWallet afterwards; never asked
     * again for this username.
     */
    void provideExisting(const QString &password);

    /**
     * Copies the password to the system clipboard directly, bypassing
     * QML's TextInput.copy() - which silently refuses to copy anything
     * when echoMode is Password, a built-in Qt anti-shoulder-surfing
     * measure that would otherwise make StatusPage.qml's "Copy" button
     * next to the masked password field a silent no-op.
     */
    void copyPasswordToClipboard() const;

Q_SIGNALS:
    void stateChanged();

private:
    void setState(State newState);

    std::unique_ptr<ISunshineWallet> m_wallet;
    std::unique_ptr<ISunshineCredentialsProcess> m_credentialsProcess;
    QString m_stateFilePath;
    QString m_lockPath;
    int m_lockTimeoutMs;
    ExecutableFinder m_executableFinder;
    PasswordGenerator m_passwordGenerator;

    State m_state = State::Unknown;
    QString m_user;
    QString m_password;
};
