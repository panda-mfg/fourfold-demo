# RSST baseline: implemented scope

The default browser baseline is now an independent Rust implementation of the
constructive RSST reduction and reconstruction procedure. DSATUR remains an
optional diagnostic. The same RSST core compiles to WebAssembly and runs in the
`fourfold-rsst` native CLI. **The implementation uses an exhaustive locator, so
it does not yet reproduce the published quadratic-time algorithm.**

The historical reference is Robertson, Sanders, Seymour, and Thomas,
[*Efficiently four-coloring planar graphs*](https://doi.org/10.1145/237814.238005),
STOC 1996, and section 6 of their full
[*The Four-Colour Theorem*](https://thomas.math.gatech.edu/PAP/fc.pdf),
J. Combinatorial Theory B 70 (1997), 2–44.

## Implemented procedures

| Paper component | Implementation |
| --- | --- |
| Plane input, section 6 | Cyclic neighbor order; paired darts; connectedness and Euler/spherical embedding checks. The exact input edge set must match the benchmark graph. |
| Triangulation | Nontriangular faces receive temporary cap vertices. The returned coloring contains only the original regions. Cap construction is timed. |
| Catalogue and projection, 3.2–3.3 | All 633 original free completions and contracts. Match both orientations, require exact interior degrees and induced interior edges, check triangular faces, and allow repeated ring vertices. |
| Contracts, 6.5 | Contract the specified edges. Preserve separating parallel edges; remove only facial digons. Invalid contractions trigger short-circuit search. |
| Tri-colorings, 2.4 | Edge colors are nonzero Klein-four differences of vertex colors. Integration back to four vertex colors checks every edge equation. |
| Consistent boundary sets, 3.1 and 6.2 | Trace bichromatic dual ribs. Enumerate every subset of boundary ribs from each retained state, deduplicating by boundary colors up to permutation. Retain the corresponding exterior coloring. |
| Configuration extension, 6.5 | Try an exterior boundary state and solve the constant-sized free completion, then reconstruct and independently check the parent coloring. Extension search is restricted to a catalogue entry, at most 26 vertices. There is no whole-input DSATUR fallback. |
| Short circuits, 6.3–6.5 | Split and glue length-2 and length-3 separators. Length-4 and length-5 branches choose the prescribed identification, diagonal, or apex construction according to the consistent boundary-color families. |
| Degree 3/4 cases | Iterative plane-minor reductions with compact undo records. At degree 4, identify a nonadjacent opposite neighbor pair; after lifting, at most three colors surround the restored vertex. |
| Validation and progress | Independently validate all output borders. Safe snapshots contain colors only for original representatives of the current minor. The color count may decrease. |

## Differences and limits

The paper's quadratic bound depends on a linear-time discharging/cartwheel
locator. This implementation scans the finite catalogue and, when necessary,
enumerates induced short cycles. The reconstruction follows RSST, but the
locator has not been replaced by the paper's procedure or given the same
complexity audit. The UI's **O(n²)** illustration refers to the publication;
measured milliseconds refer to this explicitly named research variant.

Boundary closure can retain more states than a minimal consistent set. The
implementation caps nontriangular faces with extra vertices rather than adding
only diagonals. These choices affect runtime and constants. Input checking and
fixed catalogue parsing occur before the Rust kernel timer; worker wall time
also includes dispatch, checking, and cancellation. Map generation, including
cyclic-order construction, is excluded. Both compared methods still color the
same original graph, and RSST's extra triangulation work is timed.

There is also an indexing discrepancy in the author-hosted PDF's five-cycle
paragraph on printed page 28. With `e_i = v_i v_(i+1)` and the `D_i` definition
on page 25, its stated `v2-v4, v2-v5` diagonals realize **D4**, while the text
selects **D1**. This implementation uses `v4-v1, v4-v2` for D1 and rotates that
choice for other indices. This is an inference checked by exhaustive boundary
enumeration, not a citation of a published erratum. For example, vertex colors
`[0,1,2,0,2]` satisfy the printed diagonals but give boundary class A12, which is
outside D1. The [independent finite checker](../verification/check_rsst_boundaries.py)
verifies the corrected families for all boundary colorings, all rotations,
all 15 nonempty four-ring orbit subsets, and all 1,023 five-ring orbit subsets.
See its [recorded results](../verification/results/rsst/boundary-lemmas.json).

Each trial has a shared deadline of at most 100 seconds. Each worker also has a
512-level recursion guard, an estimated 256 MiB graph-storage guard, and a
64 MiB boundary-queue guard. These are conservative implementation limits,
not a certified bound on total process memory. `resource-limit` is an incomplete
result, carries no successful timing, and is never interpreted as a
counterexample. Multiple workers can multiply memory use.

Workers use independent deterministic orders of low-degree reductions.
Variant 0 is the original ordering. Native threads share immutable input and
cooperatively cancel losing workers; browser workers have private WASM memory
and are terminated after a checked winner. Portfolio timings do not establish
the published complexity or isolate CPU scaling.

The 2026 solver is still the limited prototype described in
[full-paper-implementation.md](full-paper-implementation.md). A measured
RSST-variant/prototype comparison is not evidence for the 2026 paper's claimed
speedup. Neither the Rust code nor the complete RSST mathematical proof has
been formally verified in Lean in this repository.

## Reproduction

```sh
# Native RSST, default 2,048-region map, seed 817
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold-rsst -- --json

# Four independent native reduction orders
rust/target/release/fourfold-rsst --threads 4 --timeout 100 --json

# An embedding for another exact browser map
node demo/export-graph.cjs 4096 817 --rotation > map.rotation
rust/target/release/fourfold-rsst --input map.rotation --json

# Independent original proof-checking programs (scholarly research permission)
python3 verification/reproduce_rsst.py --fetch
python3 verification/check_rsst_boundaries.py

# Rust reconstruction tests and the actual compiled WASM engine
cargo test --manifest-path rust/Cargo.toml --offline
node demo/test_rsst.cjs
```

An optional real-browser regression script, `node demo/test_rsst_browser.cjs`,
uses an installed Playwright package and the local server at port 8000.
`FOURFOLD_URL`, `FOURFOLD_PLAYWRIGHT`, and `FOURFOLD_CHROME` can override the
URL, module location, and browser executable. It covers the default baseline,
DSATUR selection, parallel workers, exports, cancellation, and both layouts.

A rotation file starts with `n`, followed by `n` rows. Each row contains its
degree and its zero-based neighbors in cyclic order; rows are ordered by vertex
ID. Comments start with `#`. The RSST CLI accepts connected simple planar input
up to 16,384 original vertices. It checks the supplied rotation rather than
computing an embedding from an arbitrary edge list. Exit codes are 0 for a
complete coloring, 2 for an incomplete trial, and 1 for invalid input.

## Verification evidence

[Recorded proof replay](../verification/results/rsst/proof-replay.json) records
successful execution of the original reducibility checker on all 633
configurations and all five unavoidability presentations (`present7` through
`present11`). [Pinned provenance](../verification/rsst/sources.json) identifies
every original file. The unmodified C sources compile with compatibility flags;
the discharging build additionally includes `string.h`. They retain legacy
compiler warnings. These executions reproduce the finite computer checks;
they are distinct from formal verification of the new Rust implementation.

Rust tests force contraction and reconstruction for **every one of the 633
catalogue entries**, validate both embedding orientations, reject an invalid
surface rotation, and explicitly exercise separators of lengths 3, 4, and 5.
They also cover a projection with identified ring vertices and every rotated
five-ring augmentation, including the corrected D-family diagonal choice.
Parallel-edge inputs arising from reductions exercise the length-2 branch.
WASM tests cover all eight order variants on multiple maps, the 2,048-region
case with both D- and C-reductions, larger inputs, time limits, and valid
five-second partial snapshots. Every successful output is checked against the
immutable input edges.

Completing the exact published baseline still requires the discharging/pass
matcher, positive-charge hub and cartwheel locator, linear-time extraction of
a short circuit when that locator fails, and a complexity/space audit of the
whole constructive implementation. The existing original proof replay alone
does not supply those online coloring routines.
