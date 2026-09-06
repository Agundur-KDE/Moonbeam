#include "sunshinepairingresponse.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

SunshinePairingResponse::Result SunshinePairingResponse::parse(const QByteArray &body)
{
    const auto document = QJsonDocument::fromJson(body);
    if (!document.isObject()) {
        return Result::Malformed;
    }

    const auto statusValue = document.object().value(QStringLiteral("status"));
    if (!statusValue.isBool()) {
        return Result::Malformed;
    }

    return statusValue.toBool() ? Result::Success : Result::Rejected;
}
