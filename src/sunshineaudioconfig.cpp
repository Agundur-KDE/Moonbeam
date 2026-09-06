// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "sunshineaudioconfig.h"

#include <QRegularExpression>

namespace
{
QRegularExpression audioSinkLine()
{
    return QRegularExpression(QStringLiteral("^\\s*audio_sink\\s*=\\s*(\\S+)\\s*$"), QRegularExpression::MultilineOption);
}
}

bool SunshineAudioConfig::isSurroundEnabled(const QString &configContents)
{
    QString value;
    auto it = audioSinkLine().globalMatch(configContents);
    while (it.hasNext()) {
        value = it.next().captured(1);
    }
    return value == QLatin1String(SurroundSinkMonitor);
}

QString SunshineAudioConfig::withSurroundEnabled(const QString &configContents, bool enabled)
{
    QString result = configContents;
    result.remove(audioSinkLine());
    result.replace(QRegularExpression(QStringLiteral("\\n\\s*\\n")), QStringLiteral("\n"));

    if (!enabled) {
        return result;
    }

    if (!result.isEmpty() && !result.endsWith(QLatin1Char('\n'))) {
        result += QLatin1Char('\n');
    }
    result += QStringLiteral("audio_sink = %1\n").arg(QLatin1String(SurroundSinkMonitor));
    return result;
}
