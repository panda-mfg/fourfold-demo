# Computer checks

This repository contains the program for enumerating cartwheels, and combining discharging rules and cartwheels.
Each lemma can be verified by executing the following commands.

## Preparation
Build the program using CMake.
```bash
cmake -S . -B build
cmake --build build
```

Create the necessary directories. 
```bash
mkdir -p log/d{7..11} empty combined_rules/all combined_rules/non_blocked wheels/d{7..11} wheels/zero
```

Clone the repositories containing reducible configuration files and discharging rule files.
```bash
git clone git@github.com:near-linear-4ct/reducible-configurations.git
git clone git@github.com:near-linear-4ct/discharging-rules.git
```

## Lemma A.1 (for Lemma 7.1)
```bash
./build/src/main --combine_rules -R discharging-rules/R -C empty -o combined_rules/all > log/all.log &
```

## Lemma A.2 (for Lemma 7.2)
```bash
./build/src/main --combine_rules -R discharging-rules/R -C reducible-configurations/D -o combined_rules/non_blocked > log/non_blocked.log &
```
The results in `combined_rules/non_blocked` are necessary for later steps, so we can start executing the following commands after finishing this command.

## Lemma A.3 (for Lemma 8.1)
First, we enumerate wheels by considering all possible degrees of the center and its neighbors in the following command.
```bash
bash enum_possible_bad_wheels.sh
```

For each degree $d \in \{7,8,9,10,11\}$, after enumerating wheels of center degree $d$, we enumerate bad cartwheels from these wheels.
In the first step, the number of resulting wheels is decided, so we have only to execute the following commands.
```bash
bash enum_all_bad_cartwheels.sh 7 5439
bash enum_all_bad_cartwheels.sh 8 6790
bash enum_all_bad_cartwheels.sh 9 3285
bash enum_all_bad_cartwheels.sh 10 626
bash enum_all_bad_cartwheels.sh 11 8
```
The results in `wheels/zero` are necessary for later steps, so we can start executing the following commands after finishing these commands.

## Lemma A.4 (for Lemma 8.3)
```bash
./build/src/main --check_deg8 -W wheels/zero -C reducible-configurations/D > log/check_deg8.log &
```

## Lemma A.5 (for Lemma 8.5)
```bash
./build/src/main --check_7triangle -W wheels/zero -C reducible-configurations/D > log/check_7triangle.log &
```

## Lemma A.6 (for Lemma 8.6)
```bash
./build/src/main --check_deg7 -W wheels/zero -C reducible-configurations/D > log/check_deg7.log &
```