#!/bin/bash
# Install a Grove APK in a running Android emulator, launch it and check that it keeps running, then again with the
# Organic Maps look (settings key "GroveLook"), which loads other drawing rules and icons.
# Used by .github/workflows/grove-android.yaml; debug builds stop on any logged error, so this catches
# style and engine errors that release builds only log. Writes logcat and screenshots to ./smoke.
#
#   tools/grove/android_smoke_test.sh <apk>
set -euo pipefail

APK="$1"
PKG=app.organicmaps.debug
WAIT_SECONDS=60

mkdir -p smoke
adb install -r "$APK"

# Launches the app, waits and checks that it still runs. $1 names the run's screenshot and log.
run() {
  adb logcat -c
  adb shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1
  sleep "$WAIT_SECONDS"
  adb exec-out screencap -p > "smoke/$1.png" || true
  adb logcat -d > "smoke/$1-logcat.txt"
  if adb shell pidof "$PKG" > /dev/null; then
    echo "$1: the app is still running after $WAIT_SECONDS s."
    return 0
  fi
  echo "$1: the app is not running: it crashed or quit on start. Last crash lines:"
  grep -E 'FATAL|SIGABRT|Abort message|AndroidRuntime|F DEBUG|E OMaps|E organicmaps' "smoke/$1-logcat.txt" | tail -60 || true
  return 1
}

run grove
adb shell am force-stop "$PKG"
# settings.ini lives in the app's private files folder; debug builds let run-as write there.
adb shell run-as "$PKG" sh -c "'echo GroveLook=organicmaps >> files/settings.ini'"
run organicmaps-look
