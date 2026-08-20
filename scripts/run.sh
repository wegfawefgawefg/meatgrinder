#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
preset="${MG_PRESET:-dev}"
"$repo_root/scripts/build.sh" "$preset"
exec "$repo_root/build-${preset/dev/debug}/meatgrinder" "$@"

