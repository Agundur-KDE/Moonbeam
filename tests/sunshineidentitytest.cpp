#include "sunshineidentity.h"

#include <QCryptographicHash>
#include <QFile>
#include <QSslCertificate>
#include <QTemporaryDir>
#include <QTest>

namespace
{
QString fixturePath(const QString &name)
{
    return QStringLiteral(MOONBEAM_TEST_FIXTURE_DIR "/") + name;
}
}

class SunshineIdentityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void certificatePath_defaultsWhenNoCertLine();
    void certificatePath_honorsAbsoluteCertLine();
    void certificatePath_resolvesRelativeCertLineAgainstConfigDir();
    void certificateFingerprint_matchesKnownCertificate();
    void certificateFingerprint_emptyForMissingFile();
    void certificateFingerprint_emptyForMalformedFile();
    void certificateFingerprint_differsForDifferentKeysSameSubject();
};

void SunshineIdentityTest::certificatePath_defaultsWhenNoCertLine()
{
    const QString path = SunshineIdentity::certificatePath(QStringLiteral("/home/user/.config/sunshine"), QStringLiteral("port = 47989\n"));
    QCOMPARE(path, QStringLiteral("/home/user/.config/sunshine/credentials/cacert.pem"));
}

void SunshineIdentityTest::certificatePath_honorsAbsoluteCertLine()
{
    const QString path = SunshineIdentity::certificatePath(QStringLiteral("/home/user/.config/sunshine"), QStringLiteral("cert = /etc/sunshine/custom.pem\n"));
    QCOMPARE(path, QStringLiteral("/etc/sunshine/custom.pem"));
}

void SunshineIdentityTest::certificatePath_resolvesRelativeCertLineAgainstConfigDir()
{
    const QString path = SunshineIdentity::certificatePath(QStringLiteral("/home/user/.config/sunshine"), QStringLiteral("cert = mycert.pem\n"));
    QCOMPARE(path, QStringLiteral("/home/user/.config/sunshine/mycert.pem"));
}

void SunshineIdentityTest::certificateFingerprint_matchesKnownCertificate()
{
    const QByteArray fingerprint = SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("real.pem")));

    QFile file(fixturePath(QStringLiteral("real.pem")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QSslCertificate expected(file.readAll(), QSsl::Pem);
    QVERIFY(!expected.isNull());

    QCOMPARE(fingerprint, expected.digest(QCryptographicHash::Sha256));
    QCOMPARE(fingerprint.size(), 32);
}

void SunshineIdentityTest::certificateFingerprint_emptyForMissingFile()
{
    QVERIFY(SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("does-not-exist.pem"))).isEmpty());
}

void SunshineIdentityTest::certificateFingerprint_emptyForMalformedFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("garbage.pem"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not a certificate");
    file.close();

    QVERIFY(SunshineIdentity::certificateFingerprint(path).isEmpty());
}

void SunshineIdentityTest::certificateFingerprint_differsForDifferentKeysSameSubject()
{
    // The exact S-01 exploit shape: two certificates share the same subject
    // CN ("Sunshine Gamestream Host") but were issued with different keys.
    // Fingerprint comparison must reject the impostor even though a
    // CN-substring check (the old behavior) would have accepted it.
    const QByteArray real = SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("real.pem")));
    const QByteArray imposter = SunshineIdentity::certificateFingerprint(fixturePath(QStringLiteral("imposter.pem")));

    QVERIFY(!real.isEmpty());
    QVERIFY(!imposter.isEmpty());
    QVERIFY(real != imposter);
}

QTEST_GUILESS_MAIN(SunshineIdentityTest)
#include "sunshineidentitytest.moc"
