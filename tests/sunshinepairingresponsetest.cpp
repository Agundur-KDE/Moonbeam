#include "sunshinepairingresponse.h"

#include <QTest>

using SunshinePairingResponse::parse;
using SunshinePairingResponse::Result;

class SunshinePairingResponseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void statusTrueIsSuccess();
    void statusFalseIsRejected();
    void notJsonIsMalformed();
    void jsonArrayIsMalformed();
    void missingStatusFieldIsMalformed();
    void nonBooleanStatusFieldIsMalformed();
    void emptyBodyIsMalformed();
};

void SunshinePairingResponseTest::statusTrueIsSuccess()
{
    QCOMPARE(parse(R"({"status":true})"), Result::Success);
}

void SunshinePairingResponseTest::statusFalseIsRejected()
{
    QCOMPARE(parse(R"({"status":false})"), Result::Rejected);
}

void SunshinePairingResponseTest::notJsonIsMalformed()
{
    QCOMPARE(parse("<html>not json</html>"), Result::Malformed);
}

void SunshinePairingResponseTest::jsonArrayIsMalformed()
{
    QCOMPARE(parse("[1, 2, 3]"), Result::Malformed);
}

void SunshinePairingResponseTest::missingStatusFieldIsMalformed()
{
    QCOMPARE(parse(R"({"other_field":true})"), Result::Malformed);
}

void SunshinePairingResponseTest::nonBooleanStatusFieldIsMalformed()
{
    // A tampered/unexpected response using a truthy-looking non-bool must
    // not be silently coerced into Success or Rejected.
    QCOMPARE(parse(R"({"status":"true"})"), Result::Malformed);
    QCOMPARE(parse(R"({"status":1})"), Result::Malformed);
}

void SunshinePairingResponseTest::emptyBodyIsMalformed()
{
    QCOMPARE(parse(QByteArray()), Result::Malformed);
}

QTEST_GUILESS_MAIN(SunshinePairingResponseTest)
#include "sunshinepairingresponsetest.moc"
