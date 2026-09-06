// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "isunshinecredentialsprocess.h"

#include <QProcess>

bool QtSunshineCredentialsProcess::setCredentials(const QString &sunshineBin, const QString &user, const QString &password, int timeoutMs)
{
    QProcess creds;
    creds.start(sunshineBin, {QStringLiteral("--creds"), user, password});
    creds.waitForFinished(timeoutMs);
    return creds.exitStatus() == QProcess::NormalExit && creds.exitCode() == 0;
}
