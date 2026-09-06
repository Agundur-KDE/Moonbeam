// SPDX-FileCopyrightText: 2026 Agundur <info@agundur.de>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

/**
 * Sunshine's config keys that decide whether a paired Moonlight client gets
 * real mouse/keyboard/controller control of the host, or only a screen view
 * (`mouse`/`keyboard`/`controller`, config.cpp - confirmed in Sunshine's
 * source and documented in README.md). Sunshine's own default for all three
 * is full control - "Share Desktop" grants a paired client the ability to
 * operate this PC unless something explicitly turns that off (audit.txt
 * S-03).
 *
 * Pure text transforms over sunshine.conf's contents so the logic is
 * testable without touching the real file or restarting Sunshine (which is
 * the caller's job - these keys only take effect on Sunshine's next
 * start).
 */
namespace SunshineRemoteControlConfig
{
// True only when mouse, keyboard, and controller are all explicitly set to
// false - Sunshine's own default (any key absent) means full control.
bool isViewOnly(const QString &configContents);

// Returns configContents with the mouse/keyboard/controller lines rewritten
// to match viewOnly: all three set to "false" when true, or removed
// entirely (reverting to Sunshine's own defaults) when false. Any other
// config content is left untouched.
QString withViewOnly(const QString &configContents, bool viewOnly);
}
