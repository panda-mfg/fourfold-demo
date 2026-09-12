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
- Both benchmark solvers run in Rust/WebAssembly, receive the same generated map, and run sequentially in a Web Worker. The interface and map generation use JavaScript.
- Each solver trial has a **100-second limit**. Partial coloring snapshots refresh approximately **every 5 seconds**. Backtracking may undo colors; the colored count does not predict time remaining.
- **Stop** preserves the last partial map. **Results JSON** exposes completed benchmark records and their raw trial statuses.
- Final colorings are independently checked against every shared border.

## What is compared?

The measured solvers are **DSATUR exact search** and a **limited local-reduction prototype** that combines degree-at-most-four reductions, DSATUR on any remaining core, and Kempe-component restoration.

The historical panels illustrate Appel–Haken's associated quartic coloring bound and the supplied 2026 paper's claimed near-linear bound. **Neither full historical algorithm is implemented.** The Rust port preserves the prototype's scope. The growth models are not measured timings, and this demo does not constitute a formal verification of the paper. See [full-paper implementation requirements](docs/full-paper-implementation.md) for the missing components and review findings.

Solver timings include the cost of progress snapshots, but exclude WASM compilation, graph import, map generation, warm-up, final result transfer, independent checking, and drawing. Device load, browser behavior, and graph structure affect results. JSON records identify `rust-wasm` or `rust-native` as the backend.

The previous JavaScript implementation took about 59.2 seconds on one 2,048-region map with seed 817. That historical observation is not a Rust/WASM timing or a runtime guarantee. The JavaScript reference remains under `demo/reference/` for differential testing; the browser does not silently fall back to it.

## Native Rust CLI

Build and run from this repository:

```sh
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold -- --method both
```

The default input is the same 2,048-region map with seed 817 used in the browser. Progress prints to stderr every 5 seconds; completed colorings are independently validated. There are no third-party Rust crates.

```sh
# JSON results and completed color arrays
cargo run --manifest-path rust/Cargo.toml --release --bin fourfold -- --colors

# Test another size/seed using the exact browser map generator
node demo/export-graph.cjs 4096 817 > map.edges
rust/target/release/fourfold --input map.edges --method both --timeout 100 --runs 3
```

Edge-list files start with `n m`, followed by `m` undirected, zero-based vertex pairs. Comments start with `#`. Use `--input -` for stdin. Exit status is 0 when all trials complete, 2 for a timeout/unsupported trial, and 1 for invalid input. `--help` lists all options. The input must be a simple graph with at most 16,384 vertices; success always includes edge validation, but input planarity itself is not certified.

## Edit and rebuild

Edit `src/four-color-playground.html` for the interface and `rust/src/lib.rs` for both native and WebAssembly solvers. `demo/wasm-adapter.js` and `demo/benchmark-engine.js` connect the browser worker to Rust.

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
cargo test --manifest-path rust/Cargo.toml --offline
python3 build.py --check
```

The checks cover map geometry, shared borders, valid completed and partial colorings, Kempe restoration, timeouts, progress cadence, native input parsing, and serializable worker messages. WASM tests execute the actual compiled binary and compare completed colors and search counters against the JavaScript reference.

## Background

- [Appel and Haken's public account](https://celebratio.org/Appel_KI/article/674/)
- [Robin Thomas's 1998 account](https://www.ams.org/notices/199807/thomas.pdf)
- [The supplied 2026 proposal, version 2](https://arxiv.org/abs/2603.24880v2)
