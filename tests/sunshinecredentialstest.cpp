#include "sunshinecredentials.h"

#include "fakes/fakesunshinecredentialsprocess.h"
#include "fakes/fakesunshinewallet.h"

#include <QFile>
#include <QLockFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace
{
void writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    Q_UNUSED(file.open(QIODevice::WriteOnly));
    file.write(content);
}
}

class SunshineCredentialsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readyImmediatelyWhenWalletAlreadyHasCredentials();
    void generatesAndStoresFreshCredentials();
    void needsExistingPasswordWhenStateFileNamesAUser();
    void providingExistingPasswordStoresItAndBecomesReady();
    void failsClosedOnIndeterminateStateFile_neverGuessesEmpty();
    void failedWhenSunshineNotInstalled();
    void failedWhenCredentialsProcessFails_walletNeverWritten();
    void failedWhenWalletWriteFails();
    void ensureRefusesWhenLockIsHeldByAnotherProcess_parallelProcesses();
    void provideExistingRefusesWhenLockIsHeldByAnotherProcess();
};

void SunshineCredentialsTest::readyImmediatelyWhenWalletAlreadyHasCredentials()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto wallet = std::make_unique<FakeSunshineWallet>();
    wallet->hasCredentials = true;
    wallet->storedUser = QStringLiteral("moonbeam");
    wallet->storedPassword = QStringLiteral("existing-pass");
    auto process = std::make_unique<FakeSunshineCredentialsProcess>();
    FakeSunshineCredentialsProcess *processPtr = process.get();

    SunshineCredentials credentials(std::move(wallet),
                                     std::move(process),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Ready);
    QCOMPARE(credentials.user(), QStringLiteral("moonbeam"));
    QCOMPARE(credentials.password(), QStringLiteral("existing-pass"));
    QCOMPARE(processPtr->callCount, 0);
}

void SunshineCredentialsTest::generatesAndStoresFreshCredentials()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // No state file at all - SunshineCredentialsState::Fresh.

    auto walletOwned = std::make_unique<FakeSunshineWallet>();
    FakeSunshineWallet *wallet = walletOwned.get();
    auto processOwned = std::make_unique<FakeSunshineCredentialsProcess>();
    FakeSunshineCredentialsProcess *process = processOwned.get();

    SunshineCredentials credentials(std::move(walletOwned),
                                     std::move(processOwned),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("generated-pass");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Ready);
    QCOMPARE(credentials.password(), QStringLiteral("generated-pass"));
    QCOMPARE(process->callCount, 1);
    QCOMPARE(process->lastPassword, QStringLiteral("generated-pass"));
    QCOMPARE(wallet->writeCallCount, 1);
    QVERIFY(wallet->hasCredentials);
}

void SunshineCredentialsTest::needsExistingPasswordWhenStateFileNamesAUser()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString stateFile = dir.filePath(QStringLiteral("sunshine_state.json"));
    writeFile(stateFile, R"({"username":"someone"})");

    auto process = std::make_unique<FakeSunshineCredentialsProcess>();
    FakeSunshineCredentialsProcess *processPtr = process.get();

    SunshineCredentials credentials(std::make_unique<FakeSunshineWallet>(),
                                     std::move(process),
                                     stateFile,
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::NeedsExistingPassword);
    QCOMPARE(credentials.user(), QStringLiteral("someone"));
    QCOMPARE(processPtr->callCount, 0);
}

void SunshineCredentialsTest::providingExistingPasswordStoresItAndBecomesReady()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString stateFile = dir.filePath(QStringLiteral("sunshine_state.json"));
    writeFile(stateFile, R"({"username":"someone"})");

    auto walletOwned = std::make_unique<FakeSunshineWallet>();
    FakeSunshineWallet *wallet = walletOwned.get();

    SunshineCredentials credentials(std::move(walletOwned),
                                     std::make_unique<FakeSunshineCredentialsProcess>(),
                                     stateFile,
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     });

    credentials.ensure();
    QCOMPARE(credentials.state(), SunshineCredentials::State::NeedsExistingPassword);

    credentials.provideExisting(QStringLiteral("the-real-password"));

    QCOMPARE(credentials.state(), SunshineCredentials::State::Ready);
    QCOMPARE(credentials.password(), QStringLiteral("the-real-password"));
    QCOMPARE(wallet->storedUser, QStringLiteral("someone"));
    QCOMPARE(wallet->storedPassword, QStringLiteral("the-real-password"));
}

void SunshineCredentialsTest::failsClosedOnIndeterminateStateFile_neverGuessesEmpty()
{
    // audit.txt S-05 at the SunshineCredentials level: a malformed state
    // file must never be read as "fresh install" and trigger overwriting
    // real credentials.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString stateFile = dir.filePath(QStringLiteral("sunshine_state.json"));
    writeFile(stateFile, "not json at all");

    auto process = std::make_unique<FakeSunshineCredentialsProcess>();
    FakeSunshineCredentialsProcess *processPtr = process.get();
    auto walletOwned = std::make_unique<FakeSunshineWallet>();
    FakeSunshineWallet *wallet = walletOwned.get();

    SunshineCredentials credentials(std::move(walletOwned),
                                     std::move(process),
                                     stateFile,
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
    QCOMPARE(processPtr->callCount, 0);
    QCOMPARE(wallet->writeCallCount, 0);
}

void SunshineCredentialsTest::failedWhenSunshineNotInstalled()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SunshineCredentials credentials(std::make_unique<FakeSunshineWallet>(),
                                     std::make_unique<FakeSunshineCredentialsProcess>(),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QString();
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
}

void SunshineCredentialsTest::failedWhenCredentialsProcessFails_walletNeverWritten()
{
    // A crashed/erroring `sunshine --creds` must not lead to KWallet ending
    // up with a password Sunshine never actually got set to.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto process = std::make_unique<FakeSunshineCredentialsProcess>();
    process->shouldSucceed = false;
    auto walletOwned = std::make_unique<FakeSunshineWallet>();
    FakeSunshineWallet *wallet = walletOwned.get();

    SunshineCredentials credentials(std::move(walletOwned),
                                     std::move(process),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("generated-pass");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
    QCOMPARE(wallet->writeCallCount, 0);
}

void SunshineCredentialsTest::failedWhenWalletWriteFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto wallet = std::make_unique<FakeSunshineWallet>();
    wallet->writeShouldSucceed = false;

    SunshineCredentials credentials(std::move(wallet),
                                     std::make_unique<FakeSunshineCredentialsProcess>(),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     dir.filePath(QStringLiteral("creds.lock")),
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("generated-pass");
                                     });

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
}

void SunshineCredentialsTest::ensureRefusesWhenLockIsHeldByAnotherProcess_parallelProcesses()
{
    // audit.txt S-02: another Moonbeam process already holds the
    // credentials lock - this one must not also decide independently to
    // generate/set credentials.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString lockPath = dir.filePath(QStringLiteral("creds.lock"));

    QLockFile heldByAnotherProcess(lockPath);
    QVERIFY(heldByAnotherProcess.tryLock(1000));

    auto process = std::make_unique<FakeSunshineCredentialsProcess>();
    FakeSunshineCredentialsProcess *processPtr = process.get();

    SunshineCredentials credentials(std::make_unique<FakeSunshineWallet>(),
                                     std::move(process),
                                     dir.filePath(QStringLiteral("sunshine_state.json")),
                                     lockPath,
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     },
                                     200);

    credentials.ensure();

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
    QCOMPARE(processPtr->callCount, 0);
}

void SunshineCredentialsTest::provideExistingRefusesWhenLockIsHeldByAnotherProcess()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString stateFile = dir.filePath(QStringLiteral("sunshine_state.json"));
    writeFile(stateFile, R"({"username":"someone"})");
    const QString lockPath = dir.filePath(QStringLiteral("creds.lock"));

    auto walletOwned = std::make_unique<FakeSunshineWallet>();
    FakeSunshineWallet *wallet = walletOwned.get();

    SunshineCredentials credentials(std::move(walletOwned),
                                     std::make_unique<FakeSunshineCredentialsProcess>(),
                                     stateFile,
                                     lockPath,
                                     [] {
                                         return QStringLiteral("/usr/bin/sunshine-fake");
                                     },
                                     [] {
                                         return QStringLiteral("unused");
                                     },
                                     200);

    credentials.ensure();
    QCOMPARE(credentials.state(), SunshineCredentials::State::NeedsExistingPassword);

    QLockFile heldByAnotherProcess(lockPath);
    QVERIFY(heldByAnotherProcess.tryLock(1000));

    credentials.provideExisting(QStringLiteral("the-real-password"));

    QCOMPARE(credentials.state(), SunshineCredentials::State::Failed);
    QCOMPARE(wallet->writeCallCount, 0);
}

QTEST_GUILESS_MAIN(SunshineCredentialsTest)
#include "sunshinecredentialstest.moc"
