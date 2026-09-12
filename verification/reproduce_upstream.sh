#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
FOURCT_TOOL_ROOT="${FOURCT_TOOL_ROOT:-/tmp/four-color-tools}"
mkdir -p verification/build verification/results/empty
# Header-only fmt/spdlog; no source changes and no NDEBUG (assertions stay enabled).
g++ -O2 -std=c++23 -DFMT_HEADER_ONLY -DSPDLOG_FMT_EXTERNAL \
  -I"$FOURCT_TOOL_ROOT/spdlog/include" -I"$FOURCT_TOOL_ROOT/fmt/include" \
  verification/upstream/computer-checks/src/*.cpp \
  -lboost_program_options -lboost_thread -lpthread -o verification/build/computer-checks
for kind in all nonblocked; do
  FOURCT_OUTPUT="verification/results/reproduced_$kind"
  if [[ -e "$FOURCT_OUTPUT" ]]; then
    printf 'Refusing to mix old and new results: %s already exists.\n' "$FOURCT_OUTPUT" >&2
    exit 1
  fi
  mkdir -p "$FOURCT_OUTPUT"
  FOURCT_CONFIGS=verification/results/empty
  if [[ "$kind" == nonblocked ]]; then FOURCT_CONFIGS=verification/upstream/reducible-configurations/D; fi
  verification/build/computer-checks --combine_rules \
    -R verification/upstream/discharging-rules/R -C "$FOURCT_CONFIGS" \
    -o "$FOURCT_OUTPUT" > "verification/results/reproduced_$kind.log" 2>&1
done
python3 - <<'PY'
from pathlib import Path
for kind, count, charge in [('all', 1832, 8), ('nonblocked', 671, 5)]:
    folder = Path('verification/results') / ('reproduced_' + kind)
    files = list(folder.iterdir())
    assert len(files) == count, (kind, len(files))
    charges = [int(p.read_text().splitlines()[1].split()[3]) for p in files]
    assert max(charges) == charge, (kind, max(charges))
    print(kind, count, charge)
PY
