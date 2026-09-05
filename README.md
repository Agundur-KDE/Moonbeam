# Moonbeam

Kirigami companion app for [KCast](https://github.com/Agundur-KDE/KCast): share
your desktop over the network (Sunshine/Moonlight) and cast media, from one app.

## Status

Early sketch, but it builds and runs (Kirigami window, page navigation).
`SunshineController` is still a stub (naively spawns `sunshine` with no
config/pairing logic), pairing is UI-only, and media casting is a
placeholder page that will eventually reuse KCast's cast flow.

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
