#pragma once

#include <QString>

/**
 * Points Sunshine's `audio_sink` config key (its own name for "which
 * PulseAudio/PipeWire sink to capture loopback audio from" - left unset,
 * it auto-picks the default monitor, which is normally stereo) at a fixed,
 * well-known multichannel sink name Moonbeam expects the user to have
 * created themselves.
 *
 * This is deliberately the small half of a two-stage feature: it only
 * writes the config reference. It does not create the underlying PipeWire
 * sink itself (channel count, routing, cleanup) - that's a separate,
 * larger feature. Without that sink actually existing with the right
 * channel map, this config value harmlessly does nothing (Sunshine falls
 * back to auto-selection), which is why the UI pairs this toggle with an
 * explanation of what to set up first rather than presenting it as a
 * self-contained switch.
 */
namespace SunshineAudioConfig
{
// The sink name Moonbeam expects for multichannel capture. Sunshine reads
// the *monitor* of a sink for loopback, hence the ".monitor" suffix - see
// e.g. `pactl load-module module-null-sink sink_name=moonbeam-surround ...`.
constexpr auto SurroundSinkMonitor = "moonbeam-surround.monitor";

// True iff `audio_sink` is explicitly set to SurroundSinkMonitor.
bool isSurroundEnabled(const QString &configContents);

// Returns configContents with the `audio_sink` line rewritten to point at
// SurroundSinkMonitor when enabled, or removed entirely (reverting to
// Sunshine's own automatic default-monitor selection) when disabled. Any
// other config content is left untouched.
QString withSurroundEnabled(const QString &configContents, bool enabled);
}
