# Fourfold

An interactive Four Color Theorem demo with a **Rust/WebAssembly benchmark engine** and a **native Rust CLI**. Paint a map yourself, watch it being colored, and compare the measured performance of two demo solvers.

## Run

Open `index.html` in a modern browser, or serve the repository locally:

```sh
python3 -m http.server 8000
```

Then open <http://localhost:8000>. The application runs entirely in the browser. No server application, account, or API key is needed.

## Explore

- **Paint:** color regions, inspect neighboring regions, and watch a solution.
- **Compare:** choose 64, 256, 1,024, 2,048, 4,096, or 16,384 regions, a map seed, and 1, 3, or 5 timed runs.
- **CPU workers:** select Auto (up to 4), 1, 2, 4, or 8. Each worker runs Rust/WASM on the same map. Baseline and prototype trials still run one at a time, so the methods do not compete during comparison.
- Each solver trial has a **100-second limit**. Partial coloring snapshots refresh approximately **every 5 seconds**. Backtracking may undo colors; the colored count does not predict time remaining.
- **Stop** preserves the last partial map. **Results JSON** exposes completed benchmark records and their raw trial statuses.
- Final colorings are independently checked against every shared border.

## What is compared?

The default measured pair is **RSST constructive research variant** versus the
**limited local-reduction prototype** for the 2026 proposal. Select **DSATUR ·
diagnostic** to restore the earlier whole-graph search comparison.

The RSST engine uses all **633 original configurations**, their contractions,
consistent boundary-color sets, and short-circuit reconstruction in Rust. It
contains no whole-input DSATUR fallback. Its exhaustive locator differs from the
paper's linear discharging/cartwheel locator, so **this implementation does not
yet reproduce the published O(n²) guarantee**. The historical panel shows the
published bound, separately from measured times. See [RSST scope and
verification](docs/rsst-implementation.md).

The new-paper panel remains a prototype: degree-at-most-four reductions,
DSATUR on a remaining core, and Kempe restoration. The complete 2026 algorithm
is not implemented. See [full-paper requirements](docs/full-paper-implementation.md).

Browser timings measure **wall-clock latency** from dispatch to the first independently checked result, including dispatch, transfer, progress handling, winner validation, and cancellation requests. Worker creation, WASM compilation, graph import, map generation, warm-up, and final drawing are excluded. Live progress rendering and browser scheduling can affect latency. Schema-4 JSON records identify the backend, selected baseline, worker count, winning worker, individual attempt statuses, and winning worker's kernel time (`workerMs`). Counters describe the winning worker, not aggregate CPU work. These wall times have a different scope from the old kernel-only timings.

## How parallel execution works

This is a **parallel order portfolio**: workers run independent solver instances with different deterministic orders, and the first valid coloring wins. RSST workers vary low-degree reduction order; the following search-order details apply to the optional DSATUR diagnostic and local-reduction prototype. Worker 1 preserves the original static-degree/vertex-ID DSATUR order. Later workers use hashed ID ties; even-numbered workers use residual degree (currently uncolored neighbors), while odd-numbered workers retain static degree. Reduction workers also vary the initial independent-batch order. Workers do not split a shared search tree, and their partial colorings are never merged.

The browser uses separate Web Workers with private WASM memories. It works when opening `index.html` directly and with the simple local server; shared-memory WASM or cross-origin isolation headers are unnecessary. Losing workers are terminated and re-created/warmed before the next timed trial. Stop shuts down the entire pool. All workers share one trial deadline, rather than receiving consecutive 100-second budgets.

Both native CLIs support real Rust threads, an immutable shared graph, private search state, and cooperative cancellation. Parallel native times include thread startup, winner validation, and joining cancelled threads. `--threads 1` retains the original kernel timing, excluding final validation. Progress comes from worker 1 every five seconds. The API joins all threads before returning.

More workers can reduce latency on hard searches by finding a better exploration order. The improvement is not a pure CPU scaling measurement, and speedup is not guaranteed: easy reduction cases may be faster with one worker because coordination costs exceed useful work. Compare thread counts on the same size and seed; history retains the worker count for each measurement.

The previous JavaScript implementation took about 59.2 seconds on one 2,048-region map with seed 817. That historical observation is not a Rust/WASM timing or a runtime guarantee. The JavaScript reference remains under `demo/reference/` for differential testing; the browser does not silently fall back to it.

## Native RSST CLI

```sh
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold-rsst -- --threads 4 --json
node demo/export-graph.cjs 4096 817 --rotation > map.rotation
rust/target/release/fourfold-rsst --input map.rotation --timeout 100 --json
```

Default input: 2,048 regions, seed 817. Default threads: 1, preserving the original
reduction order. `--threads auto` uses up to four logical processors. Supply a
cyclic rotation file for other graphs; plain edge lists do not contain the
embedding needed by RSST. Progress appears every five seconds on stderr.
`resource-limit` and `timeout` are incomplete results. See [format, memory
limits, and algorithm differences](docs/rsst-implementation.md).

## Native diagnostic/prototype CLI

Build and run from this repository:

```sh
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold -- --method both --threads 4
```

The default input is the same 2,048-region map with seed 817 used in the browser. Thread count defaults to Auto, using up to four available logical processors. Progress prints to stderr every 5 seconds; completed colorings are independently validated. There are no third-party Rust crates.

```sh
# JSON results and completed color arrays
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold -- --colors

# Original single-thread baseline
rust/target/release/fourfold --threads 1 --json

# Test another size/seed using the exact browser map generator
node demo/export-graph.cjs 4096 817 > map.edges
rust/target/release/fourfold --input map.edges --method both --timeout 100 --runs 3
```

Edge-list files start with `n m`, followed by `m` undirected, zero-based vertex pairs. Comments start with `#`. Use `--input -` for stdin. Exit status is 0 when all trials complete, 2 for a timeout/unsupported trial, and 1 for invalid input. `--help` lists all options. The input must be a simple graph with at most 16,384 vertices; success always includes edge validation, but input planarity itself is not certified.

## Edit and rebuild

Edit `src/four-color-playground.html` for the interface and `rust/src/lib.rs` and `rust/src/rsst/` for the native and WebAssembly solvers. `demo/worker-pool.js` manages concurrency; `demo/wasm-adapter.js` and `demo/benchmark-engine.js` connect each browser worker to Rust.

For UI-only edits, rebuild using the checked-in WASM binary:

```sh
python3 build.py
```

To rebuild Rust/WASM after solver changes:

```sh
rustup target add wasm32-unknown-unknown
python3 build.py --rust
```

`src/standalone-shell.html` supplies the standalone browser wrapper. The generated page embeds the WASM bytes and works without fetching a module from a server. No npm installation, wasm-bindgen, wasm-pack, or Codex runtime is required.

## Verify

Use Node.js and Python 3:

```sh
node demo/test_benchmark.cjs
node demo/test_benchmark_progress.cjs
node demo/test_wasm.cjs
node demo/test_parallel.cjs
node demo/test_rsst.cjs
cargo test --manifest-path rust/Cargo.toml --offline
python3 build.py --check
```

The checks cover map geometry, shared borders, valid completed and partial colorings, Kempe restoration, timeouts, progress cadence, native input parsing, and serializable worker messages. WASM tests execute the compiled binary, check all eight search variants, and compare variant zero with the JavaScript reference. The pool tests use actual Node worker threads to verify 1/2/4/8 workers, independent winning-color checks, loser cancellation/restart, shared deadlines, overlap rejection, and worker-error isolation.

The original RSST finite proof checks can be replayed with
`python3 verification/reproduce_rsst.py --fetch`. All 633 reducibility cases and
all five unavoidability certificates passed; [recorded results and hashes](verification/results/rsst/proof-replay.json) are included. The C programs are fetched under their original scholarly-research permission. This is a proof-check replay, not a formal verification of the Rust code.

## Background

The complete review workspace is included alongside the current demo:

- [Original paper PDF](2603.24880v2.pdf), unchanged, with its checksum in [provenance](verification/results/provenance.json).
- [Paper review overview](docs/paper-review-overview.md) and [verification report](docs/verification-report.md).
- [Implementation plan](docs/demo-implementation-plan.md), [backlog](docs/implementation-backlog.md), and [historical algorithm comparison](docs/algorithm-comparison.md).
- [Lean proofs, checking scripts, pinned upstream sources, licenses, and recorded results](verification/README.md).
- [Original JavaScript demo snapshot](archive/original-javascript-demo/README.md) and historical browser observations.

The verification results are evidence from a **partial audit**, not a formal proof of the full paper. The recorded result summaries describe the review at the time it ran; subsequent application development does not change those proof claims. Build caches and machine-specific tool installations are not part of the repository.

- [Appel and Haken's public account](https://celebratio.org/Appel_KI/article/674/)
- [Robin Thomas's 1998 account](https://www.ams.org/notices/199807/thomas.pdf)
- [The supplied 2026 proposal, version 2](https://arxiv.org/abs/2603.24880v2)

Third-party RSST mathematical data is attributed separately in [rust/data/README.md](rust/data/README.md); the original research programs are not bundled or relicensed.
