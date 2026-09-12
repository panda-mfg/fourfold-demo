# Full paper implementation: open work

**Status: not implemented.** Rust/WASM and the native CLI implement the two existing demo methods. A successful four-coloring does not establish that the full near-linear method was used.

The target is Inoue et al., *The Four Color Theorem with Linearly Many Reducible Configurations and Near-Linear Time Coloring*, [arXiv:2603.24880v2](https://arxiv.org/abs/2603.24880v2). The requested full implementation remains an outstanding task.

## Components required by the paper

| Component | Current state | Completion requirement |
|---|---|---|
| Degree-3/4 removal and Kempe restoration | Implemented in Rust | Already exercised against the JS reference |
| 8,200 nontrivial catalogue configurations | Data available from authors; not used by the demo | Load rotations/degrees and find induced embedded matches, including flat-region configuration D2834 and exceptional D5059 |
| Structural selection, Theorem 3.3 | Missing | Full discharging rules, flat neighborhoods, non-touching configuration packing, and correctly separated obstructing-cycle packing |
| D-reducibility rank tables | Missing | Frozen-round ranks and witnesses for all ring colorings, with rank at most 25; validate the dual/primal coloring translation |
| Deterministic restoration, Section 12 | Missing | Enumerate improving component-swap choices, maintain all overlapping ring incidences, use exact conditional-expectation weights, and implement 25-pass blocks |
| Obstructing 3/4/5-cycle recursion, Section 13 | Missing | Nested cycle tree, boundary coloring representatives, added-edge/identified-vertex/star gadgets, reconstruction, and component-size accounting |
| Embedded graph representation | Missing from the Rust benchmark API | Dart rotations and planar triangulation with reversible auxiliary edges and identifications |
| End-to-end recurrence and complexity audit | Missing | Exercise both branches and verify the constant-factor reduction and accounting assumptions |

The catalogue alone cannot supply the rank tables or the complete coloring algorithm. The [authors' organization](https://github.com/near-linear-4ct) provides configuration/rule data and computational proof-checking programs. The checked public repository descriptions did not identify a ready-to-use end-to-end near-linear coloring engine.

## Corrections from the local paper review

1. **Section 12, Lemma 12.1:** non-touching configurations can share ring vertices. The two nonadjacent poles of an octahedron have the same four-vertex ring. A single host-ring array is therefore insufficient; use all ring/component incidences with deduplication and total-incidence accounting.
2. **Lemma 9.3:** arbitrary minimal homomorphic images need not preserve the single-incidence-list condition M6. Require it for the actual embedded construction and validate it after identifications.
3. **Algorithm A.1.2:** explicitly invoke DFS, return at a completed coloring, and correct vertex indexing when constructing extension tables.
4. **Rank certificates:** the inspected checker updates feasibility in place. Its sweep count is not a certified extension rank; generate levels from frozen preceding rounds or validate dependency ranks.

These corrections and missing modules are substantive implementation work. The application deliberately continues to label the reduction method as a limited prototype and exposes no misleading `paper` solver that falls back to DSATUR.

## Integration order and acceptance

1. Implement and test reversible embedded graph operations, then import and validate the complete catalogue and discharging data with pinned provenance.
2. Produce rank/witness certificates, using a resumable generator with explicit time and memory budgets; independently validate every accepted certificate.
3. Implement the corrected deterministic scheduler and both structural/recursion branches; retain independent final-color validation.
4. Exercise shared-ring, nontrivial catalogue, flat-neighborhood, nested-cycle, identification, and star-gadget fixtures. Demonstrate that each required branch runs without a fallback search replacing it.
5. Only after those checks pass, expose a separately named full-paper method in the WASM and native interfaces. Audit complexity separately from successful coloring tests.
