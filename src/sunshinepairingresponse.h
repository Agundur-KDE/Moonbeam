// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>

/**
 * Interprets Sunshine's response body to POST /api/pin, so a malformed or
 * unexpected body (audit.txt R-01: "manipulierte API-Antworten") is
 * distinguished from an explicit rejection rather than crashing or being
 * silently treated as success.
 */
namespace SunshinePairingResponse
{
enum class Result {
    Success,
    Rejected,
    Malformed,
};

// Malformed covers: not valid JSON, not a JSON object, or missing/
// non-boolean "status" field - anything that isn't a recognizable
// {"status": true|false, ...} response.
Result parse(const QByteArray &body);
}
