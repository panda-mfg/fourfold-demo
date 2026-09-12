# Four color theorem paper review

- [Verification report](verification-report.md): findings, formal counterexamples, reproduced checks, and remaining proof obligations.
- [Demo implementation plan](demo-implementation-plan.md): scope, architecture, milestones, and acceptance tests.
- [Implementation backlog](implementation-backlog.md): ordered tasks, data contracts, first-build walkthrough, and release gates.
- [Algorithm comparison and benchmark scope](algorithm-comparison.md): Appel–Haken versus the new proposal, available demo methods, timing protocol, and remaining implementation work.
- [Verification instructions](../verification/README.md): how to re-run the Lean and executable checks.

The supplied paper has **not** been fully formally verified. Selected lemmas and two counterexamples to auxiliary assertions were checked in Lean 4.19.0. Selected author computations and independent checks of all 8,200 configuration files passed. Two larger preliminary enumeration jobs timed out, and the full exhaustive proof pipeline was not run. The main theorem was not disproved by this review.

The [original PDF](../2603.24880v2.pdf) is unchanged. The [repository README](../README.md) describes the current Rust/WASM exhibit, native CLI, and parallel benchmark engine. Neither the full Appel–Haken constructive algorithm nor the complete 2026 algorithm is implemented; the historical growth models and measured demo timings are explicitly distinguished.
