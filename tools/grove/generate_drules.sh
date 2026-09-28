#!/bin/bash
# Run upstream's tools/unix/generate_drules.sh, then undo its comment-only rewrite of the priority files.
#
# kothic refreshes the "# line z8- (also has ...)" notes in data/styles/*/include/priorities_*.prio.txt
# from the compiled style, and Grove's overrides change those notes. The priorities themselves stay
# upstream's, so files whose only changes are comments are restored; anything else is reported.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
bash tools/unix/generate_drules.sh "$@"

strip_comments() { sed -E 's/[[:space:]]*#.*$//'; }
while IFS= read -r f; do
  if diff -q <(git show HEAD:"$f" | strip_comments) <(strip_comments < "$f") > /dev/null; then
    git show HEAD:"$f" > "$f"  # no index lock needed, unlike git checkout
  else
    echo "warning: $f changed beyond comments, keeping the regenerated version" >&2
  fi
done < <(git diff --name-only -- 'data/styles/*/include/priorities_*.prio.txt')
