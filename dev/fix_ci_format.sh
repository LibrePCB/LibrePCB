#!/usr/bin/env bash
set -euo pipefail

BRANCH="feature/angle-snapping"
REMOTE="origin"

# 1) ensure repo root
REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -z "$REPO_ROOT" ]; then
  echo "Error: not inside a git repo. cd to your LibrePCB repo clone and retry."
  exit 1
fi
cd "$REPO_ROOT"

echo "Fetching remote..."
git fetch "$REMOTE"

echo "Checking out branch $BRANCH..."
# create local branch if missing
if git show-ref --verify --quiet "refs/heads/$BRANCH"; then
  git checkout "$BRANCH"
  git pull "$REMOTE" "$BRANCH" || true
else
  # try to track remote branch
  if git ls-remote --exit-code --heads "$REMOTE" "$BRANCH" >/dev/null 2>&1; then
    git checkout -b "$BRANCH" "$REMOTE/$BRANCH"
  else
    echo "Remote branch $REMOTE/$BRANCH not found. Abort."
    exit 1
  fi
fi

echo "Working on $(git rev-parse --abbrev-ref HEAD) @ $(git rev-parse --short HEAD)"

CHANGES_MADE=0

if [ -x "./dev/format_code.sh" ]; then
  echo "Running official formatter: ./dev/format_code.sh --all"
  ./dev/format_code.sh --all
  # If the script modified files, they will show in git status below
else
  echo "Warning: ./dev/format_code.sh not found or not executable; running fallback normalization steps."
fi

# Fallback normalization (safe, non-functional edits):
# - normalize line endings to LF
# - remove trailing spaces
# - replace tabs with 4 spaces for text/source files (skip third-party dirs)
# - ensure newline at EOF
# NOTE: these operate only on tracked files

# List tracked files to process (exclude some vendor/3rd-party paths used by CI)
EXCLUDE_PATHS="^libs/polyclipping/|^share/librepcb/licenses/|^LICENSES/|^dist/"
FILES=$(git ls-files)

process_count=0
for f in $FILES; do
  # skip binary files
  if file --brief --mime-type "$f" | grep -qv text; then
    continue
  fi
  # skip excluded paths
  echo "$f" | grep -Eq "$EXCLUDE_PATHS" && continue

  # normalize CRLF -> LF
  if grep -q $'\r' "$f"; then
    perl -pi -e 's/\r\n?/\n/g' "$f"
  fi

  # remove trailing spaces
  perl -pi -e 's/[ \t]+$//' "$f"

  # replace tabs with 4 spaces (safe in most source files). Avoid binary and UI files.
  # skip .ui and binary-like extensions
  case "$f" in
    *.ui|*.png|*.jpg|*.jpeg|*.svgz|*.ico|*.so|*.dll|*.dylib) ;;
    *)
      # replace tabs only if file contains any tabs
      if grep -q $'\t' "$f"; then
        perl -0777 -pe 's/\t/    /g' -i.orig "$f" && rm -f "$f.orig"
      fi
      ;;
  esac

  # ensure newline at EOF
  tail -c1 "$f" | od -An -t u1 | grep -qE "^$" || printf "\n" >> "$f"

  ((process_count++))
done

echo "Normalized $process_count text files (fallback step)."

# Fix accidental executable bit for non-scripts: make non-shebang files non-executable.
# Find tracked files flagged executable and without a shebang line.
EXEC_FILES=$(git ls-files --stage | awk '$1 == "100755" { print $4 }' || true)
for f in $EXEC_FILES; do
  # check if file starts with shebang
  if head -n1 "$f" | grep -q '^#!'; then
    echo "Keeping exec bit for script $f"
  else
    echo "Removing exec bit for $f"
    git update-index --chmod=-x "$f"
  fi
done

# Now add and commit if anything changed
if ! git diff --quiet || ! git diff --cached --quiet; then
  git add -A
  git commit -m "ci: apply formatting/whitespace normalization (automated)"
  CHANGES_MADE=1
  echo "Committed formatting changes."
else
  echo "No changes to commit."
fi

# Push changes if any
if [ "$CHANGES_MADE" -eq 1 ]; then
  echo "Pushing to $REMOTE/$BRANCH..."
  git push "$REMOTE" "$BRANCH"
  echo "Pushed. CI should run automatically now."
else
  echo "Nothing changed; no push performed."
fi

# Optionally run check step if format script exists
if [ -x "./dev/format_code.sh" ]; then
  echo "Running check: ./dev/format_code.sh --all --check"
  set +e
  ./dev/format_code.sh --all --check
  rc=$?
  set -e
  if [ $rc -ne 0 ]; then
    echo "Note: format check failed (exit code $rc). Inspect the working tree and re-run ./dev/format_code.sh --all, then commit+push the results."
  else
    echo "Format check passed locally."
  fi
fi

echo "Done."
