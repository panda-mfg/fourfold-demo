#!/bin/bash
set -exuo pipefail
cd "$(dirname "$0")"

for d in $(seq 7 11); do
    ./build/src/main --enum_wheels -d $d -R discharging-rules/R -C reducible-configurations/D -S combined_rules/non_blocked -o wheels/d$d > log/wheels_d$d.log &
done
