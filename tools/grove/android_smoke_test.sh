#!/bin/bash
# Install a Grove APK in a running Android emulator, download the world map, open the map and check that the app
# keeps running, then again with the Organic Maps look (settings key "GroveLook"), which loads other drawing rules and
# icons. The map shows the Alps at zoom 7, so it draws the world map's features, land cover and relief.
# Used by .github/workflows/grove-android.yaml; debug builds stop on any logged error, so this catches
# style and engine errors that release builds only log. Writes logcat and screenshots to ./smoke.
#
#   tools/grove/android_smoke_test.sh <apk>
set -euo pipefail

APK="$1"
PKG=app.organicmaps.debug
WAIT_SECONDS=60
DOWNLOAD_SECONDS=300

mkdir -p smoke
adb install -r "$APK"
# No permission prompts: one would stay on top of the app's task, and a relaunch would only bring it back.
for permission in ACCESS_FINE_LOCATION ACCESS_COARSE_LOCATION POST_NOTIFICATIONS; do
  adb shell pm grant "$PKG" "android.permission.$permission" || true
done

running() { adb shell pidof "$PKG" > /dev/null; }

# Taps the "Download" button of the first start's world map screen.
tap_download() {
  adb shell uiautomator dump /sdcard/ui.xml > /dev/null
  local bounds
  bounds=$(adb shell cat /sdcard/ui.xml | grep -oE 'btn_download_resources"[^>]*' | grep -oE 'bounds="[^"]*"' || true)
  if [[ -z "$bounds" ]]; then
    echo "No download button on screen."
    return 1
  fi
  read -r x1 y1 x2 y2 <<< "$(grep -oE '[0-9]+' <<< "$bounds" | tr '\n' ' ')"
  adb shell input tap $(((x1 + x2) / 2)) $(((y1 + y2) / 2))
}

map_shown() { adb shell dumpsys activity activities | grep -E 'topResumedActivity|mResumedActivity' | grep -q MwmActivity; }

wait_for_map() {
  local waited=0
  until map_shown; do
    if ((waited >= DOWNLOAD_SECONDS)) || ! running; then
      return 1
    fi
    sleep 5
    waited=$((waited + 5))
  done
}

# Launches the app, opens the map and checks that it still runs a while later. $1 names the run's screenshot and log.
run() {
  adb logcat -c
  # -S stops the app first, so each run starts it anew.
  adb shell am start -S -W -n "$PKG/app.organicmaps.DownloadResourcesActivity"
  sleep 5
  if ! map_shown; then
    tap_download || true
  fi
  if wait_for_map; then
    adb shell am start -W -a android.intent.action.VIEW -d "geo:47.3,11.4?z=7" "$PKG"
    sleep "$WAIT_SECONDS"
  fi
  adb exec-out screencap -p > "smoke/$1.png" || true
  adb logcat -d > "smoke/$1-logcat.txt"
  if running && map_shown; then
    echo "$1: the map is shown and the app is still running after $WAIT_SECONDS s."
    return 0
  fi
  if running; then
    echo "$1: the app runs but doesn't show the map: the world map download failed or took over $DOWNLOAD_SECONDS s."
  else
    echo "$1: the app is not running: it crashed or quit. Last crash lines:"
  fi
  grep -E 'FATAL|SIGABRT|Abort message|AndroidRuntime|F DEBUG|E OMaps|E organicmaps|E OMcore' "smoke/$1-logcat.txt" |
    tail -60 || true
  return 1
}

run grove
adb shell am force-stop "$PKG"
# settings.ini lives in the app's private files folder; debug builds let run-as write there.
adb shell run-as "$PKG" sh -c "'echo GroveLook=organicmaps >> files/settings.ini'"
run organicmaps-look
