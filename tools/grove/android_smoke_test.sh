#!/bin/bash
# Install a Grove APK in a running Android emulator, launch it and check that it keeps running.
# Used by .github/workflows/grove-android.yaml; debug builds stop on any logged error, so this catches
# style and engine errors that release builds only log. Writes logcat and a screenshot to ./smoke.
#
#   tools/grove/android_smoke_test.sh <apk>
set -euo pipefail

APK="$1"
PKG=app.organicmaps.debug
WAIT_SECONDS=60

mkdir -p smoke
adb install -r "$APK"
adb logcat -c
adb shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1
sleep "$WAIT_SECONDS"
adb exec-out screencap -p > smoke/screen.png || true
adb logcat -d > smoke/logcat.txt

if adb shell pidof "$PKG" > /dev/null; then
  echo "The app is still running after $WAIT_SECONDS s."
  exit 0
fi

echo "The app is not running: it crashed or quit on start. Last crash lines:"
grep -E 'FATAL|SIGABRT|Abort message|AndroidRuntime|F DEBUG|E OMaps|E organicmaps' smoke/logcat.txt | tail -60 || true
exit 1
