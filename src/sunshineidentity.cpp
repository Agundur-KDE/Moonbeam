// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "sunshineidentity.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSslCertificate>

QString SunshineIdentity::certificatePath(const QString &configDir, const QString &configContents)
{
    static const QRegularExpression certLine(QStringLiteral("^\\s*cert\\s*=\\s*(.+?)\\s*$"), QRegularExpression::MultilineOption);

    QString configuredPath;
    auto it = certLine.globalMatch(configContents);
    while (it.hasNext()) {
        // Sunshine itself takes the last occurrence when a key repeats.
        configuredPath = it.next().captured(1);
    }

    if (configuredPath.isEmpty()) {
        return QDir(configDir).filePath(QStringLiteral("credentials/cacert.pem"));
    }

    if (QFileInfo(configuredPath).isAbsolute()) {
        return configuredPath;
    }

    return QDir(configDir).filePath(configuredPath);
}

QSslCertificate SunshineIdentity::loadCertificate(const QString &certPath)
{
    QFile file(certPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QSslCertificate();
    }

    return QSslCertificate(file.readAll(), QSsl::Pem);
}

QByteArray SunshineIdentity::certificateFingerprint(const QString &certPath)
{
    const QSslCertificate certificate = loadCertificate(certPath);
    if (certificate.isNull()) {
        return {};
    }

    return certificate.digest(QCryptographicHash::Sha256);
}

bool SunshineIdentity::matchesPinnedFingerprint(const QSslCertificate &certificate, const QByteArray &pinnedFingerprint)
{
    if (pinnedFingerprint.isEmpty()) {
        return false;
    }

    return certificate.digest(QCryptographicHash::Sha256) == pinnedFingerprint;
}
