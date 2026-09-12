# Fourfold

An interactive Four Color Theorem demo: paint a map yourself, watch it being colored, and compare the measured performance of two demo solvers.

## Run

Open `index.html` in a modern browser, or serve the repository locally:

```sh
python3 -m http.server 8000
```

Then open <http://localhost:8000>. The application runs entirely in the browser. No server application, account, or API key is needed.

## Explore

- **Paint:** color regions, inspect neighboring regions, and watch a solution.
- **Compare:** choose 64, 256, 1,024, 2,048, 4,096, or 16,384 regions, a map seed, and 1, 3, or 5 timed runs.
- Both solvers receive the same generated map and run sequentially in a Web Worker.
- Each solver trial has a **100-second limit**. Partial coloring snapshots refresh approximately **every 5 seconds**. Backtracking may undo colors; the colored count does not predict time remaining.
- **Stop** preserves the last partial map. **Results JSON** exposes completed benchmark records and their raw trial statuses.
- Final colorings are independently checked against every shared border.

## What is compared?

The measured solvers are **DSATUR exact search** and a **limited local-reduction prototype** that combines degree-at-most-four reductions, DSATUR on any remaining core, and Kempe-component restoration.

The historical panels illustrate Appel–Haken's associated quartic coloring bound and the supplied 2026 paper's claimed near-linear bound. **Neither full historical algorithm is implemented.** The growth models are not measured timings, and this demo does not constitute a formal verification of the paper.

Solver timings include the cost of progress snapshots, but exclude map generation, warm-up, final result transfer, independent checking, and drawing. Device load, browser behavior, and graph structure affect results.

For one measured 2,048-region map with seed 817, DSATUR completed in about 59.2 seconds. This is a single observation, not a runtime guarantee.

## Edit and rebuild

Edit `src/four-color-playground.html` for the interface and `demo/benchmark-engine.js` for the solver worker. Rebuild the standalone page with Python 3:

```sh
python3 build.py
```

`src/standalone-shell.html` supplies the standalone browser wrapper. No npm installation or Codex runtime is required to rebuild.

## Verify

Use Node.js and Python 3:

```sh
node demo/test_benchmark.cjs
node demo/test_benchmark_progress.cjs
python3 build.py --check
```

The checks cover map geometry, shared borders, valid completed and partial colorings, Kempe restoration, timeouts, progress cadence, and serializable worker messages.

## Background

- [Appel and Haken's public account](https://celebratio.org/Appel_KI/article/674/)
- [Robin Thomas's 1998 account](https://www.ams.org/notices/199807/thomas.pdf)
- [The supplied 2026 proposal, version 2](https://arxiv.org/abs/2603.24880v2)
