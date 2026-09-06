#pragma once

#include <QString>
#include <optional>

/**
 * Parses Sunshine's configured web UI port safely (audit.txt S-06):
 * the old code cast an unchecked toUInt() straight to quint16 and added 1
 * without checking for overflow, so a `port` value above 65535 would be
 * silently truncated, and exactly 65535 would wrap the web UI port to 0 -
 * either way, Moonbeam could end up checking (and pairing against) the
 * wrong local port instead of refusing to guess.
 */
namespace SunshinePortConfig
{
// Sunshine's own default base port (config.cpp).
constexpr quint16 DefaultBasePort = 47989;

// The web UI port Sunshine derives as (configured base `port`, or
// DefaultBasePort if unconfigured) + 1 (confighttp::PORT_HTTPS offset).
// Returns std::nullopt if a `port` line is present but not a valid,
// in-range base port (unparsable, negative, or >= 65535 - the top value
// would overflow when adding 1) - callers must treat that as "the
// configuration is invalid", never fall back to guessing a default.
std::optional<quint16> resolveWebUiPort(const QString &configContents);
}
