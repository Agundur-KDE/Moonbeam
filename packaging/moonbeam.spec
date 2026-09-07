Name:           moonbeam
Version:        0.1.3
Release:        1%{?dist}
Summary:        Wireless screen sharing for KDE Plasma via Sunshine/Moonlight

License:        GPL-3.0-or-later
URL:            https://github.com/Agundur-KDE/Moonbeam

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  gettext
BuildRequires:  extra-cmake-modules
BuildRequires:  qt6-base-devel
BuildRequires:  qt6-declarative-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-kconfig-devel
BuildRequires:  kf6-knotifications-devel
BuildRequires:  kf6-kwallet-devel

Requires:       plasma6-workspace

%description
Moonbeam manages Sunshine and pairs Moonlight clients with one click,
turning KDE Plasma into a wireless presenter for meetings, classrooms,
and conference rooms. Not a media-casting app — for casting local/remote
media to a Chromecast, see KCast.

Source0: _service

%prep

rm -rf ./*

shopt -s nullglob
picked=""
for d in %{_sourcedir}/moonbeam-* %{_sourcedir}/Moonbeam-* %{_sourcedir}/moonbeam ; do
  if [ -d "$d" ] && [ -f "$d/CMakeLists.txt" ]; then
    picked="$d"
    break
  fi
done

if [ -n "$picked" ]; then
  cp -a "$picked"/. .
else
  for f in %{_sourcedir}/* ; do
    base="$(basename "$f")"
    case "$base" in
      *.spec|*.dsc|*.changes|*.obsinfo|_service|service_attic|screenshot|*.patch)
        continue ;;
    esac
    cp -a "$f" .
  done
fi

%build
%cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DKDE_INSTALL_USE_QT_SYS_PATHS=ON \
  -DKDE_INSTALL_QMLDIR=%{_qt6_qmldir} \
  -DKDE_INSTALL_PLUGINDIR=%{_qt6_plugindir}
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/moonbeam
%{_datadir}/applications/org.agundur.moonbeam.desktop
%{_datadir}/icons/hicolor/512x512/apps/org.agundur.moonbeam.png
%{_datadir}/locale/*/LC_MESSAGES/moonbeam.mo

%changelog
* Mon Sep 07 2026 Alec <info@agundur.de> - 0.1.3-1
- Fixed rpmlint hard-fail (badness 10001, build aborted): the .desktop
  file was installed via install(PROGRAMS ...), setting the executable bit
  on a non-script file (E: script-without-shebang) — switched to
  install(FILES ...). The installed binary also carried a $ORIGIN-relative
  RPATH meant for running out of the build tree (E: binary-or-shlib-
  defines-rpath) — added set(CMAKE_SKIP_INSTALL_RPATH TRUE). Also escaped
  a literal "%prep" in the 0.1.0 changelog entry below, which rpmlint's
  macro scanner flagged (same landmine KCast hit once, see its own
  changelog history) — not a build blocker this time, fixed proactively.
  Verified installed file perms (644) and RPATH (empty) locally before
  re-submitting.

* Mon Sep 07 2026 Alec <info@agundur.de> - 0.1.2-1
- Fixed CMake configure failure on the OBS build root: CMakeLists.txt set
  QT_DEFAULT_MAJOR_VERSION only after include(KDEInstallDirs), which
  already needs QT_MAJOR_VERSION to pick KDEInstallDirs6.cmake over the
  Qt5 variant — it defaulted to Qt5 and failed looking for a qmake5
  executable that doesn't exist on a Qt6-only system. Now sets
  QT_MAJOR_VERSION=6 before the include. Verified with a full local build
  on openSUSE Tumbleweed before re-submitting to OBS.

* Mon Sep 07 2026 Alec <info@agundur.de> - 0.1.1-1
- Packaging-only fix: obs-submit.yml now runs `osc add` before commit,
  since a freshly-bootstrapped OBS package (no prior files) was silently
  skipped by `osc commit` ("nothing to do"), never publishing spec/
  _service. No functional changes over 0.1.0.

* Mon Sep 07 2026 Alec <info@agundur.de> - 0.1.0-1
- Initial packaging: RPM build via OBS, mirrors the kfritz/kcast spec
  pattern (obs_scm _service, source-dir auto-detection during prep).
