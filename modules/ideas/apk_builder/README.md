# APK builder

Build an APK with your settings as its defaults: the app exports its switches, look and custom style as a small
file, and a GitHub Action (workflow_dispatch) builds the APK from it, recording the exact source commit so the build
can be repeated. A second button builds plain Organic Maps from the upstream commit Grove is based on
(modules/look/tools/classic_base.txt). Phones can't build an APK themselves; the file travels by share sheet.
