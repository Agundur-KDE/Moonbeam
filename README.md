# Moonbeam

Kirigami companion app for [KCast](https://github.com/Agundur-KDE/KCast): share
your desktop over the network (Sunshine/Moonlight) and cast media, from one app.

## Status

Builds and runs. `SunshineController` is a QML singleton (see "Why a
singleton" below) that:
- detects whether Sunshine is installed, already running, or stopped, and
  never starts a second instance against an already-open web UI port
- verifies a candidate port actually belongs to Sunshine (its self-signed
  cert's CN, "Sunshine Gamestream Host") before trusting it or sending it
  credentials, rather than assuming any process on that port is Sunshine
- reads the real web UI port from Sunshine's own config (`port` + 1) instead
  of hardcoding 47990
- pairs a Moonlight client via a real `POST /api/pin` call

Media casting is still a placeholder page for KCast's existing flow.

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

## Roadmap ideas

- **Prerequisites check, not a setup wizard.** Rely on the system's
  `sunshine` (`Requires: sunshine` in the RPM) instead of bundling/building
  our own — avoids the ~15min Sunshine+FFmpeg build entirely. But a package
  dependency alone doesn't cover non-RPM installs (Flatpak, source builds),
  and "installed" isn't the same as "safe to start": we hit a real
  credential-overwrite bug today because two Sunshine processes shared the
  same `~/.config/sunshine` state dir, and Sunshine's ports (47989/47990/...)
  aren't shareable either. So on startup, check: is `sunshine` on PATH, is
  an instance already running (which port), and surface that as one plain
  status line/message ("Sunshine not found — install with ...", "Sunshine
  already running on port X, using it") — not a multi-step wizard, just the
  same check we need internally anyway before deciding whether to spawn a
  new process or attach to an existing one.
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
Config, Notifications).

```sh
cmake -B build -S .
cmake --build build
```
