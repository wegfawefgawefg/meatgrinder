#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
"$repo_root/scripts/build.sh" dev
ctest --test-dir "$repo_root/build-debug" --output-on-failure
SDL_VIDEODRIVER=dummy "$repo_root/build-debug/meatgrinder" --smoke

