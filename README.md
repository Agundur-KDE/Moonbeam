# Moonbeam

Kirigami companion app for [KCast](https://github.com/Agundur-KDE/KCast):
share your desktop over the network via Sunshine/Moonlight.

Single-purpose on its own - media casting stays KCast's job. The plan is
for KCast to grow a "Share Desktop" launcher that starts Moonbeam, not for
Moonbeam to reimplement or embed KCast's cast flow; two overlapping cast
UIs in two apps would just compete with each other.

## Status

Builds and runs. `SunshineController` is a QML singleton (see "Why a
singleton" below) that:
- detects whether Sunshine is installed, already running, or stopped, and
  never starts a second instance against an already-open web UI port
- verifies a candidate port actually belongs to Sunshine by pinning the
  exact SHA-256 fingerprint of the certificate Sunshine is configured to
  serve (`SunshineIdentity`), not just a CN string any local process could
  put in a self-signed certificate of its own, before trusting it or
  sending it credentials
- shows a "View only" switch (`SunshineRemoteControlConfig`) for turning off
  a paired client's mouse/keyboard/controller control - Sunshine grants full
  input control by default
- reads the real web UI port from Sunshine's own config (`port` + 1) instead
  of hardcoding 47990
- pairs a Moonlight client via a real `POST /api/pin` call
- manages the web UI credentials itself via KWallet (`SunshineCredentials`) -
  generates and stores them on first use, or asks once for an existing
  password Moonbeam didn't set - never a form field a human has to fill in
  every time
- refreshes its status every 5s so the UI notices when an externally-started
  Sunshine instance disappears on its own
- has a real About dialog (`Kirigami.AboutPage` + `KAboutData`) and uses
  KDE's i18n pipeline (`i18n()` in QML, not Qt's `qsTr()`) - "Moonbeam" the
  app name itself is deliberately left untranslated

Single page app now (Status + a pushed Pairing page) - no more drawer
navigation now that media casting isn't Moonbeam's job.

### Known limitations (not yet fixed)

- `stop()` blocks the GUI thread for up to 3s waiting for Sunshine to exit
  before falling back to `kill()`. Acceptable for an explicit user click on
  a sketch-stage app; a polished version should do this asynchronously.
- The TOCTOU window between the port check and spawning a process is
  narrowed by a `QLockFile` (`start()`) but not eliminated for instances
  started by something other than Moonbeam at the exact same moment.
- `pair()`'s PIN validation lives only in the QML `IntValidator` plus
  Sunshine's own server-side check - there's no redundant validation in
  `SunshineController` itself.
- The web UI password is briefly visible as a `sunshine --creds` process
  argument when generating fresh credentials, and is held as a QML-visible
  `SunshineCredentials.password` property - Sunshine's own CLI has no
  stdin/file alternative to the former, and fixing the latter would need
  `SunshineController` to reach into `SunshineCredentials` for pairing
  directly rather than being handed the password from QML, which risks
  reintroducing the two-instances-racing-to-generate-credentials hazard
  `SunshineCredentials` was built to avoid. Documented in code at both
  sites (audit.txt S-04, S-07).

## Roadmap ideas

- **KCast launcher button (lives in KCast's repo, not here).** KCast should
  get a "Share Desktop" action that spawns `moonbeam` the same way it
  already spawns `catt` (Plasma5Support's "executable" engine), plus a
  "Moonbeam not installed" fallback message - the same prerequisites-check
  philosophy as Moonbeam's own Sunshine check, just one level up.
- **RPM packaging.** `Requires: sunshine` instead of bundling/building our
  own Sunshine fork — avoids the ~15min Sunshine+FFmpeg build entirely. The
  prerequisites check itself (is `sunshine` on PATH, already running, which
  port, is it actually Sunshine) is already implemented as an inline
  StatusPage message rather than a setup wizard - what's still missing is
  the actual `.spec` file (see KCast's `packaging/kcast.spec` for the
  pattern) and CI to build it. A package dependency still won't cover
  non-RPM installs (Flatpak, source builds) - those get the same runtime
  check, just without the OS nudging the user to install Sunshine upfront.
- **Capture scope.** Which physical output to share (`output_name` in
  Sunshine's config, e.g. `HDMI-A-1` vs `DP-1`) is trivial — already proven
  live. Sharing a single window or an arbitrary screen region is not
  implemented in Sunshine's `kwingrab.cpp` today, but the underlying KWin
  protocol (`zkde_screencast_unstable_v1`) already has the primitives for
  it: `stream_window` (by window UUID) and `stream_region` (x/y/width/height)
  requests exist since protocol v2/v3, `kwingrab.cpp` currently only calls
  `stream_output` (whole-display capture). Adding window/region picking
  would mean wiring one of those two requests into `kwingrab.cpp` plus a
  picker UI (e.g. a Spectacle-style rectangle selector) here in Moonbeam.

## Why a singleton, not a per-page controller

`SunshineController` used to be instantiated inside each QML page that
needed it (`SunshineController { id: sunshine }` in both `StatusPage.qml`
and `PairingPage.qml`). That was a real bug, not just a style issue: each
instance owns a live `QProcess`, and navigating away from a page could
destroy its `SunshineController` - silently killing a Sunshine process that
instance had started, just from switching pages. It's now `QML_SINGLETON`,
so there's exactly one controller for the app's lifetime.

## Why a separate app, not a Plasmoid

Sunshine is a long-running background service with its own tray icon and
multi-step pairing flow — a poor fit for a Plasmoid's panel-tied lifecycle.
See the design discussion in the KCast project memory for the full
reasoning (KWin screencast authorization, `capture=kwin` vs `capture=portal`,
why a Kirigami app was chosen over plain Qt).

## Build

Requires Qt6, Extra CMake Modules (ECM), and KF6 (CoreAddons, I18n, Kirigami,
Config, Notifications, Wallet).

```sh
cmake -B build -S .
cmake --build build
```
