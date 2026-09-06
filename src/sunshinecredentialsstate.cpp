// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#include "sunshinecredentialsstate.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

SunshineCredentialsState::ProbeResult SunshineCredentialsState::probeConfiguredUsername(const QString &stateFilePath)
{
    QFile file(stateFilePath);
    if (!file.exists()) {
        return {Result::Fresh, {}};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return {Result::Indeterminate, {}};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {Result::Indeterminate, {}};
    }

    const QJsonObject object = document.object();
    const auto usernameValue = object.value(QStringLiteral("username"));
    if (!usernameValue.isString()) {
        // No "username" field at all (or the wrong type) - an unrecognized
        // shape, not evidence of a fresh install.
        return {Result::Indeterminate, {}};
    }

    const QString username = usernameValue.toString();
    if (username.isEmpty()) {
        // Sunshine itself writes an empty username to mean "not configured
        // yet" - a positively identified fresh state.
        return {Result::Fresh, {}};
    }

    return {Result::ExistingUser, username};
}
