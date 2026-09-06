// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

/**
 * Reads Sunshine's own state file to figure out whether it already has web
 * UI credentials configured - without ever confusing "we couldn't tell" for
 * "there aren't any" (audit.txt S-05).
 *
 * SunshineCredentials::ensure() only auto-generates and sets new
 * credentials via `sunshine --creds` when this reports Fresh. Missing
 * permissions, a different Sunshine version/format, or a partially written
 * file must never be silently treated as "no credentials yet" - that would
 * let ensure() overwrite a real existing password.
 */
namespace SunshineCredentialsState
{
enum class Result {
    // The state file doesn't exist at all - a genuine first run, safe to
    // auto-generate credentials for.
    Fresh,
    // The state file is valid and names a configured username.
    ExistingUser,
    // The file exists but couldn't be read, parsed, or didn't look like a
    // Sunshine state file we recognize - unknown, must not be treated as
    // "empty".
    Indeterminate,
};

struct ProbeResult {
    Result result = Result::Indeterminate;
    QString username;
};

ProbeResult probeConfiguredUsername(const QString &stateFilePath);
}
