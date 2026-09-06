// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "sunshineremotecontrolconfig.h"

#include <QRegularExpression>

namespace
{
QRegularExpression keyLine(const QString &key)
{
    return QRegularExpression(QStringLiteral("^\\s*%1\\s*=\\s*(\\S+)\\s*$").arg(key), QRegularExpression::MultilineOption);
}

bool isExplicitlyFalse(const QString &configContents, const QString &key)
{
    QString value;
    auto it = keyLine(key).globalMatch(configContents);
    while (it.hasNext()) {
        value = it.next().captured(1);
    }
    return value.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0;
}

QString withoutKeyLines(QString configContents, const QString &key)
{
    configContents.remove(keyLine(key));
    // keyLine() only removes the key=value text itself (the line's
    // leading/trailing \s* is inside the pattern, but the trailing
    // newline is not); drop now-blank lines it leaves behind.
    configContents.replace(QRegularExpression(QStringLiteral("\\n\\s*\\n")), QStringLiteral("\n"));
    return configContents;
}
}

bool SunshineRemoteControlConfig::isViewOnly(const QString &configContents)
{
    return isExplicitlyFalse(configContents, QStringLiteral("mouse")) && isExplicitlyFalse(configContents, QStringLiteral("keyboard"))
        && isExplicitlyFalse(configContents, QStringLiteral("controller"));
}

QString SunshineRemoteControlConfig::withViewOnly(const QString &configContents, bool viewOnly)
{
    QString result = configContents;
    for (const QString &key : {QStringLiteral("mouse"), QStringLiteral("keyboard"), QStringLiteral("controller")}) {
        result = withoutKeyLines(result, key);
    }

    if (!viewOnly) {
        return result;
    }

    if (!result.isEmpty() && !result.endsWith(QLatin1Char('\n'))) {
        result += QLatin1Char('\n');
    }
    result += QStringLiteral("mouse = false\nkeyboard = false\ncontroller = false\n");
    return result;
}
