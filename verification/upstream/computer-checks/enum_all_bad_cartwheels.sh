#!/bin/bash
set -exuo pipefail
cd "$(dirname "$0")"

# inputs
degree=$1
n=$2

# settings
if [[ "$OSTYPE" == "darwin"* ]]; then
    MAX_JOBS=$(sysctl -n hw.ncpu) # Mac
else
    MAX_JOBS=$(nproc) # Linux
fi
CHECK_INTERVAL=1

# main process
echo "Start enumerating bad cartwheels for degree $degree."

for idx in $(seq 0 $((n - 1))); do
    while [ $(jobs -p | wc -l) -ge $MAX_JOBS ]; do
        sleep $CHECK_INTERVAL
    done

    echo "Starting ${idx}/${n}"
    wheel="wheels/d${degree}/d${degree}_${idx}.cartwheel"
    ./build/src/main --enum_cartwheels -w ${wheel} -C reducible-configurations/D -R discharging-rules/R -S combined_rules/non_blocked -o wheels/zero > log/d${degree}/enum_d${degree}_${idx}.log &
done

# wait for all background jobs to finish
wait
echo "All jobs completed."
