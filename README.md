<div align="center">
  <img src="assets/icons/org.agundur.moonbeam.png" width="120" alt="Moonbeam icon">
  <h1>Moonbeam</h1>
  <p><strong>Switch presenters, not cables.</strong><br>
  Wireless screen sharing for KDE Plasma — start sharing, enter a PIN on the
  other device, and you're presenting.</p>
</div>

<p align="center">
  <img src="Moonbeam.png" width="420" alt="Moonbeam sharing a KDE Plasma desktop">
</p>

Sharing a laptop screen in a meeting, classroom, or conference room
shouldn't mean searching for the right cable, the right adapter, or
whether the app you're about to show even has a "cast" button. Moonbeam
shares your KDE Plasma desktop wirelessly with any [Moonlight](https://moonlight-stream.org/)
device — phone, tablet, TV, projector, another laptop — no matter what's
on screen: a game, a terminal, a website that refuses to cooperate,
professional software with no casting support built in. If it can be
displayed, Moonbeam can share it.

That makes switching between presenters fast: whoever's turn it is opens
Moonbeam, clicks **Start Sharing**, and the next person pairs in with a
PIN — no crawling under the table for a different dongle every time.
Pairing is protected, so you connect to the intended computer, not
another device on the same Wi-Fi network.

Under the hood, Moonbeam manages [Sunshine](https://github.com/LizardByte/Sunshine),
the open-source continuation of NVIDIA GameStream — a real-time,
hardware-encoded streaming host, not a buffered casting protocol. Moonbeam
handles it so you don't have to: you never touch Sunshine's own web UI,
generate a certificate, or manage a password by hand.

It's a companion app for [KCast](https://github.com/Agundur-KDE/KCast),
not a replacement — media casting (YouTube, Spotify, etc. to a Chromecast)
stays KCast's job, this is single-purpose by design. The plan is for KCast
to grow a "Share Desktop" launcher that starts Moonbeam, not for the two
to duplicate each other's cast flow.

## How it works

1. Open Moonbeam and click **Start Sharing**.
2. Open [Moonlight](https://moonlight-stream.org/) on the receiving
   device — [Android](https://play.google.com/store/apps/details?id=com.limelight&pli=1),
   iOS, Windows, macOS, Linux, or a smart TV all work.
3. First time on that device: enter the PIN Moonbeam shows you.
4. Present. Click **Stop Sharing** when you're done, or just hand off to
   the next presenter.

## Features

- **Present without cables.** Share your screen over the local network —
  no HDMI cable, no USB-C dongle hunt.
- **Switch presenters quickly.** The next person pairs in with a PIN
  instead of unplugging and reconnecting the room's display setup.
- **Works with anything on your screen.** The app, website, game, or
  presentation doesn't need its own casting feature.
- **Connects to the right computer.** Moonbeam verifies the device you
  pair with, so you don't end up connected to another machine on the
  same Wi-Fi network by mistake.
- **Secure sign-in handled for you.** Moonbeam creates and stores the
  connection credentials automatically — nothing to type into a password
  field.
- **View only**, for when you want to present without handing over
  mouse/keyboard/controller control of your computer.
- **Surround audio (5.1/7.1)** for setups with a proper receiver, not
  just stereo. See the in-app "?" button for the one-time setup this needs.
- **Translated**: English (default), German, French, Spanish, Russian.

### Note: View only / Surround audio only take effect on restart

Both switches are only clickable while not currently sharing, and
changing one while already sharing has no effect until the next restart.
Watch out for one thing in particular: the same button reads "Stop
Sharing" while sharing and "Start Sharing" once stopped, so it's easy to
reflexively click that spot twice (stop, then immediately start again)
before the switches become interactable. The sequence that actually
works:

1. Click **Stop Sharing** and wait for the status to read "Not sharing."
2. Toggle **View only** / **Surround audio** now, while stopped.
3. Click **Start Sharing** again to apply.

## Requirements

- KDE Plasma (Linux) as the sending computer.
- [Sunshine](https://github.com/LizardByte/Sunshine) installed — Moonbeam
  manages it, but doesn't install it for you.
- [Moonlight](https://moonlight-stream.org/) on whatever device you're
  sharing *to*.

## Security

- Before trusting anything as "your" Sunshine, Moonbeam checks its exact
  certificate against a pinned fingerprint — not just a name, which any
  local process could fake.
- Credentials are generated once and stored in KWallet, not typed in or
  left lying around in a config file.
- Moonbeam never starts a second Sunshine instance against one that's
  already running.

This isn't a formal security guarantee for every scenario — see
[Technical details](#technical-details) and [`audit.txt`](audit.txt) for
the full threat model, what's covered, and what isn't.

## License

GPL v3, see [`LICENSE`](LICENSE) *(or the license header in individual
source files)*.

---

## Technical details

For contributors, or anyone who wants to know exactly how this works.

### What is Sunshine/Moonlight?

Two separate pieces, from the same open-source project family:

- **[Sunshine](https://github.com/LizardByte/Sunshine)** runs on the machine
  you want to share *from* - the "host". It captures your screen and audio
  and streams them out. Moonbeam manages Sunshine for you on this side:
  detects whether it's installed and running, starts/stops it safely, and
  never touches Sunshine's own web UI credentials by hand.
- **[Moonlight](https://moonlight-stream.org/)** is the client that
  connects *to* Sunshine and displays the stream. It runs on almost
  anything: [Android](https://play.google.com/store/apps/details?id=com.limelight&pli=1),
  iOS, Windows, macOS, Linux, smart TVs, and some Android-based projectors
  (including this project's own test setup, a JMGO smart beamer).

Moonbeam itself only ever runs on the **host** side - install Moonlight
separately on whatever device you want to view/control the stream from.

### Implementation notes on the Features above

- **Safe process management.** Detects whether Sunshine is installed,
  already running, or stopped, and never starts a second instance against
  an already-open web UI port - refreshes every 5s so the UI notices when
  an externally-started instance disappears on its own.
- **Certificate pinning, not a CN check.** Before trusting a port as
  actually being Sunshine (and, critically, before ever sending it
  credentials), Moonbeam pins the exact SHA-256 fingerprint of the
  certificate Sunshine is configured to serve - not just a Common Name
  string, which any local process could put in a self-signed certificate
  of its own.
- **Zero-touch credentials.** Web UI credentials are generated and stored
  in KWallet on first use, or asked for once if Sunshine already had a
  password Moonbeam didn't set - never a form field to fill in every time.
  An on-page "Web UI Login" display (masked, with copy buttons) covers the
  one case you do need them: the browser's own Basic-Auth prompt.
- **Real pairing**, via Sunshine's actual `POST /api/pin` API - enter the
  PIN Moonlight shows you, done.
- **View only.** A paired Moonlight client gets full mouse/keyboard/
  controller control of the host by default (Sunshine emulates real input
  devices) - not just a screen view. One switch turns that off for
  presenting/screen-sharing use cases.
- **Surround audio (5.1/7.1).** Points Sunshine at a multichannel audio
  sink instead of the default stereo monitor - see the in-app "?" button
  for the one-time PipeWire setup this needs. Transports discrete
  multichannel PCM (Opus), not a Dolby Atmos object-audio bitstream - your
  receiver gets real 5.1/7.1 channels, not Atmos metadata.
- **Open Web UI**, a direct link to Sunshine's own settings (apps, display/
  output choice, video/audio options) that Moonbeam deliberately doesn't
  duplicate.

### Build

Requires Qt6, Extra CMake Modules (ECM), and KF6 (CoreAddons, I18n, Kirigami,
Config, Notifications, Wallet).

```sh
cmake -B build -S .
cmake --build build
```

### Tests

`SunshineController` and `SunshineCredentials` reach the real Sunshine
process, sockets, KWallet, and filesystem only through injected interfaces
(`ISunshineProcess`, `ISunshineNetworkProbe`, `ISunshineWallet`,
`ISunshineCredentialsProcess`) - their public, QML-visible constructors wire
up the real ones; a second constructor on each takes fakes instead, so
`sunshinecontrollertest`/`sunshinecredentialstest` can exercise cert
imitation, wrong ports, invalid config, parallel-process lock contention,
wallet failures, process crashes/hangs, and tampered pairing responses
without touching a real process, socket, wallet daemon, or file.

```sh
ctest --test-dir build --output-on-failure
```

A full security audit is in [`audit.txt`](audit.txt).

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
  argument when generating fresh credentials (Sunshine's CLI has no
  stdin/file alternative), and is held as a QML-visible property so it can
  reach `pair()`'s Basic-Auth header and the on-page display. Documented in
  code at both sites, see `audit.txt` S-04/S-07 for the full reasoning.

### Roadmap ideas

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
- **Surround audio, part two.** Moonbeam currently only points Sunshine's
  `audio_sink` at a well-known sink name - it doesn't create the
  multichannel PipeWire sink itself. A future version could set that up
  automatically instead of asking the user to run one `pactl` command by
  hand.

### Why a singleton, not a per-page controller

`SunshineController` used to be instantiated inside each QML page that
needed it (`SunshineController { id: sunshine }` in both `StatusPage.qml`
and `PairingPage.qml`). That was a real bug, not just a style issue: each
instance owns a live process handle, and navigating away from a page could
destroy its `SunshineController` - silently killing a Sunshine process that
instance had started, just from switching pages. It's now `QML_SINGLETON`,
so there's exactly one controller for the app's lifetime.

### Why a separate app, not a Plasmoid

Sunshine is a long-running background service with its own tray icon and
multi-step pairing flow — a poor fit for a Plasmoid's panel-tied lifecycle.
See the design discussion in the KCast project memory for the full
reasoning (KWin screencast authorization, `capture=kwin` vs `capture=portal`,
why a Kirigami app was chosen over plain Qt).
