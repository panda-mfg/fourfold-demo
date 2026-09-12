# Original JavaScript demo snapshot

These files preserve the initial single-worker JavaScript implementation, its tests, historical browser measurements, and original export helper. The active Rust/WASM engine, native Rust CLI, and parallel worker pool are documented in the [repository README](../../README.md).

The unchanged `sync_benchmark.py` is a historical helper tied to the original local conversation paths. Use `python3 build.py` from the repository root to rebuild the current demo.

To run the archived solver tests from the repository root:

```sh
node archive/original-javascript-demo/test_benchmark.cjs
node archive/original-javascript-demo/test_benchmark_progress.cjs
```

The original worker is also retained under `demo/reference/` for the current Rust/WASM differential tests. Recorded JavaScript timings are not measurements of the parallel Rust engine.
