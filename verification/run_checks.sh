#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
FOURCT_TOOL_ROOT="${FOURCT_TOOL_ROOT:-/tmp/four-color-tools}"
FOURCT_LEAN="${FOURCT_LEAN:-$FOURCT_TOOL_ROOT/lean-4.19.0-linux/bin/lean}"
mkdir -p verification/results
"$FOURCT_LEAN" --version > verification/results/lean_version.txt
"$FOURCT_LEAN" verification/PaperAudit.lean > verification/results/lean.log 2>&1
if rg -q 'sorryAx|error:' verification/results/lean.log; then
  cat verification/results/lean.log
  exit 1
fi
python3 verification/check_data.py > verification/results/independent_checks.log
printf 'Lean proofs and independent data checks passed.\n'
