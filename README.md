# Moonbeam

Kirigami companion app for [KCast](https://github.com/Agundur-KDE/KCast): share
your desktop over the network (Sunshine/Moonlight) and cast media, from one app.

## Status

Early sketch. Not buildable end-to-end yet — `SunshineController` is a stub,
pairing is UI-only, and media casting is a placeholder page that will
eventually reuse KCast's cast flow.

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
