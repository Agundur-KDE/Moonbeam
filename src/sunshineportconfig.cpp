#include "sunshineportconfig.h"

#include <QRegularExpression>

std::optional<quint16> SunshinePortConfig::resolveWebUiPort(const QString &configContents)
{
    // Captures whatever follows "port =" verbatim - including garbage that
    // isn't a number at all - so it can be positively identified as
    // invalid below, rather than silently falling through to the default
    // as if no `port` line had been present.
    static const QRegularExpression portLine(QStringLiteral("^\\s*port\\s*=\\s*(\\S+)\\s*$"), QRegularExpression::MultilineOption);

    QString rawValue;
    auto it = portLine.globalMatch(configContents);
    while (it.hasNext()) {
        rawValue = it.next().captured(1);
    }

    if (rawValue.isEmpty()) {
        return static_cast<quint16>(DefaultBasePort + 1);
    }

    bool ok = false;
    const qlonglong basePort = rawValue.toLongLong(&ok);
    // The web UI port is basePort + 1: basePort must leave room for that
    // without overflowing quint16, so 65535 itself is out of range here.
    if (!ok || basePort < 0 || basePort >= 65535) {
        return std::nullopt;
    }

    return static_cast<quint16>(basePort + 1);
}
