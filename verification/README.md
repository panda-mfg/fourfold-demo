# Verification evidence

This directory contains **partial formal verification and a reproducibility audit**, not a formal proof of the paper's main theorem. See `../docs/verification-report.md` for the findings and exact scope.

## Re-run the formal and independent checks

From the repository root:

```bash
bash verification/run_checks.sh
```

The default tool installation for this session is `/tmp/four-color-tools`. It is temporary. For a different installation, set `FOURCT_LEAN` to a Lean 4.19.0 executable and `FOURCT_TOOL_ROOT` to the dependency directory. `check_data.py` uses only the Python standard library. The Lean file imports `Std`, uses no Mathlib, adds no axioms, and uses neither `sorry` nor `native_decide`. Its axiom audit reports only Lean's standard `propext` and `Quot.sound` where needed. The exact finite color argument uses no axioms.

`PaperAudit.lean` verifies arithmetic consequences, Kempe recoloring under an explicit closure hypothesis, octahedron ring overlap, and a minimal dart image that fails the single-list property. It does not formalize planarity, the full catalogue's reducibility, unavoidability, or the complete recursive algorithm.

`check_data.py` independently parses all 8,200 configurations, checks selected D0-D2 conditions, enumerates cycle color classes, checks an oriented octahedron witness, and exercises the proposed incidence-list repair with exact rational weights. These Python checks are not proof-assistant results. In particular, D0 checks follow the actual outer facial walk through cut vertices, not just the set of boundary edges.

## Re-run the upstream rule checks

```bash
bash verification/reproduce_upstream.sh
```

Requires g++, Boost program_options/thread, fmt 11.2.0 headers and spdlog 1.15.3 headers. Headers are currently in `/tmp/four-color-tools/{fmt,spdlog}/include`. The script compiles the pinned C++ sources without changing them, with assertions enabled, and checks output counts and maximum charges directly. This header-only build bypasses the upstream CMake dependency discovery and GoogleTest download. The upstream unit-test suite was not run.

Upstream sources and MIT licenses are preserved in `upstream/`. Each snapshot has an `UPSTREAM_COMMIT.json`; the PDF checksum and all repository commits are recorded in `results/provenance.json`. These are repository snapshots fetched during this review, not claimed to be the authors' exact May 2026 build environment.

The wheel-generation commands used the same executable with `--enum_wheels -d D -R verification/upstream/discharging-rules/R -C verification/upstream/reducible-configurations/D -S verification/results/combined_nonblocked -o verification/results/wheels_dD`, for D from 7 to 11. Each invocation had a 180-second limit. Check `results/wheels_summary.json` for success versus timeout; initial wheel enumeration is only a preliminary stage of Lemma A.3.

For the full unavoidability pipeline, follow the pinned `upstream/computer-checks/README.md`: complete every cartwheel job and then A.4-A.6. Those full runs were not performed in this review. A successful rule count or initial wheel count does not establish the later lemmas.

## Sample reducibility checks

`prepare_planar.cpp` is a local adapter that calls upstream table generators for planar chains only, up to ring size 14. The original CLI also generates other surfaces. The two sample `.conf` files are converted by the original `-d` option, then checked with the mandatory `-l` flag (the default is projective).

The reducibility checker catches exceptions and can return exit code zero on error, so results must contain the affirmative success message and no critical/error messages. Iteration counts from its in-place update loop are **not certified Kempe levels**. No sample success certifies the full 8,200-item catalogue or D3's bound of 25.

The sample builds used these commands from the project root:

```bash
FOURCT_TOOL_ROOT=/tmp/four-color-tools
mkdir -p verification/build
g++ -O2 -std=c++20 -DFMT_HEADER_ONLY -DSPDLOG_FMT_EXTERNAL \
  -I"$FOURCT_TOOL_ROOT/spdlog/include" -I"$FOURCT_TOOL_ROOT/fmt/include" \
  -include fmt/ranges.h \
  verification/upstream/reducibility_checker/main.cpp \
  verification/upstream/reducibility_checker/coloring.cpp \
  -lboost_program_options -lpthread -o verification/build/reducibility-checker
g++ -O2 -std=c++20 -DFMT_HEADER_ONLY -DSPDLOG_FMT_EXTERNAL \
  -I"$FOURCT_TOOL_ROOT/spdlog/include" -I"$FOURCT_TOOL_ROOT/fmt/include" \
  -Iverification/upstream/reducibility_checker \
  verification/prepare_planar.cpp verification/upstream/reducibility_checker/coloring.cpp \
  -lpthread -o verification/build/prepare-planar
mkdir -p verification/results/reducibility_samples
cd verification/results/reducibility_samples
../../build/prepare-planar
for name in D0000 D2834; do
  ../../build/reducibility-checker -i "../../upstream/reducible-configurations/D/$name.conf" -d > "$name.dconf"
  ../../build/reducibility-checker -i "$name.dconf" -l > "$name.log" 2>&1
done
```

Inspect the logs for `Graph is D-reducible!` and absence of errors. The recorded runs used equivalent executable paths under `/tmp/four-color-tools`; their two checks passed. The adapter and the added include are explicitly outside the unchanged upstream sources.

## RSST historical baseline

The independent RSST Rust research implementation is described in
[its scope document](../docs/rsst-implementation.md). Original finite proof
checks passed for all 633 configurations and all five unavoidability
presentations; [logs and pinned source hashes](results/rsst/proof-replay.json)
are included. Reproduce them with `python3 verification/reproduce_rsst.py --fetch`
from the repository root. The original C programs are fetched under their
scholarly-research permission into ignored build storage.

`python3 verification/check_rsst_boundaries.py` independently enumerates the
small consistent boundary sets and reconstruction families, including the
documented D-family indexing correction. These are executable checks. The
Rust solver and the complete RSST proof have not been formally verified here.
