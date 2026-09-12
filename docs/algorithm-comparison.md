# Historical comparison and runnable benchmark scope

The requested historical pair is **Appel–Haken's first computer-assisted four-color proof** and **the supplied 2026 near-linear coloring proposal**. Neither complete constructive coloring algorithm is implemented in this workspace. The current demo therefore does **not** provide a measured Appel–Haken-versus-2026 runtime comparison.

It now provides two explicitly distinguished features:

1. Historical growth models for the requested pair, with source links and no invented milliseconds.
2. Actual browser timings for the available DSATUR solver and a limited local-reduction prototype, on identical generated maps of 64, 256, 1,024, 2,048, 4,096, or 16,384 regions. The 2,048-region case uses a 64 × 32 seed grid; the other cases use square grids.

## Historical identification

Appel and Haken announced the proof in 1976; their full two-part work appeared in 1977, with John Koch as a coauthor of Part II. See the [original authors' public account](https://celebratio.org/Appel_KI/article/674/) and the [Part II publication](https://projecteuclid.org/journals/illinois-journal-of-mathematics/volume-21/issue-3/Every-planar-map-is-four-colorable-Part-II-Reducibility/10.1215/ijm/1256049012.full).

Robin Thomas's [1998 account](https://www.ams.org/notices/199807/thomas.pdf) describes the associated Appel–Haken coloring algorithm as quartic and the later Robertson–Sanders–Seymour–Thomas algorithm as quadratic. The supplied [2026 paper, v2](https://arxiv.org/abs/2603.24880v2), claims O(n log n), improving on the 1996/97 quadratic result. It does not use DSATUR as that historical predecessor.

The original proof-checking programs test finite reducibility cases. Their execution time is not the same quantity as the time to color a newly supplied n-vertex graph. Reproducing the old proof-checking computation would therefore not establish an n-versus-coloring-time curve for Appel–Haken.

The display uses the illustrative functions `(n/64)^4` and `n log2(n)/(64 log2(64))`, each set to 1 at n=64. These compare growth shapes only. Big-O bounds do not provide leading constants, wall-clock predictions, or measured speedup ratios. The reviewed 2026 proof also retains the unresolved obligations documented in the [verification report](verification-report.md).

## Runnable methods

This document records the original JavaScript benchmark and its timing protocol. For the current Rust/WASM, native CLI, and parallel worker behavior, see the [repository README](../README.md). The original source is [benchmark-engine.js](../archive/original-javascript-demo/benchmark-engine.js).

- **Demo DSATUR:** exact backtracking; choose an uncolored vertex with the most distinct neighbor colors, breaking ties by its original graph degree and then vertex ID. An explicit stack avoids recursion-depth failures. Neighbor-color counts are maintained incrementally. This implements the same selection strategy as the small public exhibit but scales its data handling.
- **Local reduction prototype:** repeatedly choose a non-touching batch of active vertices of degree at most 4; remove it; solve any leftover degree-at-least-5 core with DSATUR; restore in reverse order. When four distinct colors block a vertex, try complete bichromatic components until a swap frees a color. Every final result is checked independently against the immutable input edges.

The prototype has no certified near-linear complexity. It omits the paper's full configuration catalogue, discharging, separator branch, rank certificates, and coordinated conditional-expectation scheduler. Its simple reductions are shared mathematical ingredients, not an implementation of the entire new proposal. Neither runnable method is Appel–Haken's algorithm.

## Inputs and generation

Both methods receive precisely the same adjacency lists and seed. Regions are bounded Voronoi cells of a jittered square grid; all regions are connected convex polygons. Corners do not create adjacency. Borders retain their generating neighbor identities during clipping, and tests independently reconstruct adjacency from shared geometric segments.

The 5×5 clipping stencil has a geometric justification: each seed lies within 0.25 of its unit-square center in each coordinate. Every point of the domain is within `sqrt(2) × 0.75` of some seed. Any competing seed three or more grid columns or rows away is at least 2.5 away from the given seed, exceeding twice that covering radius, and cannot define its cell boundary. Tests additionally compare the resulting cells with all seeds on smaller instances.

This is a restricted benchmark family. In particular, the tested generated maps often peel completely using the simple prototype. They must not be used to infer behavior on arbitrary planar graphs, the performance of the full catalogue/flat-region algorithm, or the ratio between the historical algorithms.

## Timing protocol

- Two warm-up runs of each method on a fixed 64-region map precede each benchmark.
- Run 1, 3, or 5 trials per method; alternate method order and execute serially in a Web Worker to avoid direct contention between the two solvers.
- Start the measured interval before solver-specific allocation and processing. Include all reductions, core search, Kempe searches, swaps, and restoration.
- Exclude input generation, warm-up, final worker transfer, independent output checking, and rendering. Show map generation separately. Solver time includes the small cost of copying and sending periodic progress snapshots; progress drawing runs separately on the UI thread and may still contend for device resources.
- Report the median and range of completed trials. Retain every raw trial and status in the results JSON. Browser JIT, scheduling, timer resolution, and device load still affect these observations.
- Stop a solver trial after its 100-second budget. Skip further trials of that method after a failure/timeout. A timeout is censored, not a zero, solved instance, or proof of impossibility; no cross-method speedup is calculated from incomplete trials.
- Send current partial colorings at approximately 5-second intervals from solver checkpoints, plus a final safe snapshot on timeout. The UI independently checks assigned colors and borders before displaying each snapshot, leaving uncolored regions blank. Backtracking can reduce the colored count; it is not a completion percentage or time estimate. Intermediate Kempe swaps are never published. Final success still requires a separate check of the entire coloring.
- The Stop action terminates the worker and retains the last partial map with a stopped label. Completed prior benchmark records remain available; an interrupted benchmark is not added as a completed record.

The interface preserves the original Paint mode, adds Compare mode, supports all-size sweeps, and exposes results as selectable JSON. Its historical implementation status is included explicitly in the exported data.

## Validation and reproducibility

Run the engine checks with Node:

```sh
node archive/original-javascript-demo/test_benchmark.cjs
node archive/original-javascript-demo/test_benchmark_progress.cjs
```

The checks cover exact map geometry, reconstructed borders, final colorings, shared-ring octahedron input, an icosahedron with a nonempty search core, intermediate Kempe invariants, unsupported nonplanar K5, and timeout reporting. Browser checks additionally exercise the largest size, cancellation and restart, mode switching, the four-region painting puzzle, and responsive light/dark layouts.

The engine suite checks 37 generated maps, 51,685 geometrically reconstructed shared borders, and 1,175 Kempe swaps, in addition to its fixed fixtures. The progress suite uses a deterministic clock to check the 100-second default budget, 5-second snapshot cadence, valid partial colorings, safe Kempe updates, independent snapshot copies, and request correlation. A prior five-size browser sweep is saved in [browser benchmark observations](../demo/browser-benchmark-observations.json), including its original budgets, raw timings, timeout statuses, and environment metadata. These are observations of the two demo methods only.

A [browser run with live progress](../demo/browser-benchmark-progress-observation.json) on 2,048 regions, seed 817, completed DSATUR in 59.159 seconds with 11 snapshots at 5-second intervals; its 5,953 shared borders passed validation. The reduction prototype completed in 14.2 ms on that run. The browser checks also passed partial-map display, cancellation, restart, and desktop/light and mobile/dark rendering. These are single-run observations on the recorded environment, not expected times for every device or seed.

The original conversation export helper is preserved in [the JavaScript archive](../archive/original-javascript-demo/README.md). Rebuild the current standalone Rust/WASM demo with `python3 build.py` from the repository root.

That script names this task's durable visualization file explicitly. It does not install, deploy, or host a website. Benchmark results vary by environment and should be regenerated where the exhibit will run.

## Work required for the requested full measured comparison

1. Pin the exact historical constructive algorithm and distinguish the original publications from later corrections and polynomial-time extraction. Acquire and audit the matching configuration data and executable reduction/reconstruction rules; a reducibility-proof checker alone is insufficient.
2. Implement and audit the full 2026 algorithm, including the corrections and certificate gates already listed in the review and implementation backlog.
3. Add separately named full-method adapters to the common benchmark harness. Give both implementations the same embedded graph, input transformations, hardware/runtime treatment, and limits. Preserve separate offline-preprocessing and online-coloring measurements.
4. Validate each returned coloring independently; then benchmark multiple graph families and seeds, including instances requiring the difficult catalogue and separator branches.

Until those gates pass, the UI must continue to mark full historical runtimes unavailable. The existing demo timings cannot fill those fields.
