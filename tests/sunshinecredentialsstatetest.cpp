#include "sunshinecredentialsstate.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using SunshineCredentialsState::probeConfiguredUsername;
using SunshineCredentialsState::Result;

namespace
{
QString writeFile(QTemporaryDir &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    Q_UNUSED(file.open(QIODevice::WriteOnly));
    file.write(content);
    return path;
}
}

class SunshineCredentialsStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void missingFileIsFresh();
    void validUsernameIsExistingUser();
    void emptyUsernameFieldIsFresh();
    void unreadableFileIsIndeterminate();
    void malformedJsonIsIndeterminate();
    void jsonWithoutUsernameFieldIsIndeterminate();
};

void SunshineCredentialsStateTest::missingFileIsFresh()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto probe = probeConfiguredUsername(dir.filePath(QStringLiteral("does-not-exist.json")));
    QCOMPARE(probe.result, Result::Fresh);
}

void SunshineCredentialsStateTest::validUsernameIsExistingUser()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("state.json"), R"({"username":"alice"})");
    const auto probe = probeConfiguredUsername(path);
    QCOMPARE(probe.result, Result::ExistingUser);
    QCOMPARE(probe.username, QStringLiteral("alice"));
}

void SunshineCredentialsStateTest::emptyUsernameFieldIsFresh()
{
    // Sunshine's own state file uses an empty username string to mean "not
    // configured yet" - a recognized, positively-identified fresh state,
    // not a read/parse failure.
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("state.json"), R"({"username":""})");
    const auto probe = probeConfiguredUsername(path);
    QCOMPARE(probe.result, Result::Fresh);
}

void SunshineCredentialsStateTest::unreadableFileIsIndeterminate()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("state.json"), R"({"username":"alice"})");
    QFile file(path);
    QVERIFY(file.setPermissions(QFile::Permissions()));

    // Root (or a permissive test sandbox) can bypass permission bits - only
    // assert Indeterminate when the file is genuinely unreadable to us.
    if (file.open(QIODevice::ReadOnly)) {
        QSKIP("Cannot make the fixture file unreadable in this environment (running as root?)");
    }

    const auto probe = probeConfiguredUsername(path);
    QCOMPARE(probe.result, Result::Indeterminate);
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
}

void SunshineCredentialsStateTest::malformedJsonIsIndeterminate()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("state.json"), "not json at all");
    const auto probe = probeConfiguredUsername(path);
    QCOMPARE(probe.result, Result::Indeterminate);
}

void SunshineCredentialsStateTest::jsonWithoutUsernameFieldIsIndeterminate()
{
    // A valid JSON object, but not one containing a recognizable
    // "username" field at all (different Sunshine version/format, or a
    // partially written file) - must not be read as "empty".
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("state.json"), R"({"unrelated_field":true})");
    const auto probe = probeConfiguredUsername(path);
    QCOMPARE(probe.result, Result::Indeterminate);
}

QTEST_GUILESS_MAIN(SunshineCredentialsStateTest)
#include "sunshinecredentialsstatetest.moc"
