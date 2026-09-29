#!/bin/bash
# Merge the latest Organic Maps into the current branch, then regenerate compiled files.
#
#   modules/core/tools/sync_upstream.sh               # fetch + merge organicmaps/master + regenerate
#   modules/core/tools/sync_upstream.sh --regenerate  # only regenerate (after resolving conflicts by hand)
#
# Generated files are never merged by hand: on conflict upstream's copy is taken and then
# rebuilt from the merged sources, so Grove's overrides are applied again.
set -euo pipefail

UPSTREAM_REMOTE="${UPSTREAM_REMOTE:-organicmaps}"
UPSTREAM_BRANCH="${UPSTREAM_BRANCH:-master}"

cd "$(git rev-parse --show-toplevel)"

# Outputs of modules/look/tools/generate_drules.sh and modules/look/tools/generate_symbols.py.
is_generated() {
  case "$1" in
    data/drules_*.bin | data/drules_*.txt | data/colors.txt | data/patterns.txt | \
    data/visibility.txt | data/classificator.txt | data/types.txt | data/symbols/*/*/symbols.* | \
    data/symbols-classic/*/*/symbols.* | modules/look/tools/classic_base.txt) return 0 ;;
    *) return 1 ;;
  esac
}

regenerate() {
  # The Organic Maps look is built from the upstream commit merged last.
  if git rev-parse --verify -q "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" > /dev/null; then
    git merge-base HEAD "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" > modules/look/tools/classic_base.txt
  fi
  if ! python3 -c 'import sys; sys.exit(sys.version_info < (3, 10))'; then
    echo "Needs Python 3.10+ as python3 on PATH." >&2
    exit 1
  fi
  echo "Updating submodules..."
  git submodule update --init --recursive --depth 1
  echo "Regenerating drawing rules..."
  modules/look/tools/generate_drules.sh > /dev/null
  echo "Regenerating icons with Grove colors..."
  python3 modules/look/tools/generate_symbols.py > /dev/null
  local changed=()
  while IFS= read -r f; do
    is_generated "$f" && changed+=("$f")
  done < <(git diff --name-only -- data modules/look/tools/classic_base.txt; git ls-files --others --exclude-standard -- data)
  if [ ${#changed[@]} -gt 0 ]; then
    git add -- "${changed[@]}"
    git commit -s -m "[styles] Regenerated drules and symbols"
  else
    echo "Generated files already up to date."
  fi
}

if [ "${1:-}" = "--regenerate" ]; then
  regenerate
  exit 0
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "Commit or stash your changes before syncing." >&2
  exit 1
fi

git fetch "$UPSTREAM_REMOTE" "$UPSTREAM_BRANCH"
UPSTREAM_REF="$UPSTREAM_REMOTE/$UPSTREAM_BRANCH"
if git merge-base --is-ancestor "$UPSTREAM_REF" HEAD; then
  echo "Already contains $UPSTREAM_REF."
  exit 0
fi

echo "Merging $UPSTREAM_REF ($(git rev-list --count HEAD.."$UPSTREAM_REF") new commits)..."
if ! git merge --no-ff --no-edit "$UPSTREAM_REF"; then
  unresolved=()
  while IFS= read -r f; do
    if is_generated "$f"; then
      git checkout --theirs -- "$f"
      git add -- "$f"
    else
      unresolved+=("$f")
    fi
  done < <(git diff --name-only --diff-filter=U)
  if [ ${#unresolved[@]} -gt 0 ]; then
    echo >&2
    echo "Resolve these conflicts, commit the merge, then run: $0 --regenerate" >&2
    printf '  %s\n' "${unresolved[@]}" >&2
    exit 1
  fi
  git commit --no-edit
fi

regenerate
