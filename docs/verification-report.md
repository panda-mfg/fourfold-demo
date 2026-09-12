# Review and formal verification report

**Verdict: the supplied paper cannot be certified as formally verified as written.** This review found two false auxiliary assertions, checked counterexamples in Lean, and found defects in the printed coloring pseudocode. Several supporting lemmas and computational results passed verification. These findings do **not** disprove the main near-linear coloring theorem; the two auxiliary errors have plausible local repairs, which still need to be incorporated into a complete proof.

## Source and scope

The reviewed document is *The Four Color Theorem with Linearly Many Reducible Configurations and Near-Linear Time Coloring*, Yuta Inoue, Ken-ichi Kawarabayashi, Atsuyuki Miyashita, Bojan Mohar, Carsten Thomassen, and Mikkel Thorup, arXiv:2603.24880v2. The local PDF has 92 physical pages: the abstract page and printed pages 1-91. Its cover is dated May 8, 2026; the arXiv version was submitted May 7. References below use **printed page numbers**, so printed p.49 is physical PDF page 50.

All sections and the appendix were read as extracted text. Key figures and the pages containing the main findings were also inspected as rendered pages. The review traced the proof dependencies, inspected relevant C++ routines, reproduced selected author computations, ran independent data checks, and built a small Lean formalization. It did not check every line of C++ against every pseudocode step, re-prove every topological argument, or execute all exhaustive jobs.

The PDF SHA-256 is `27016693b9799d2ccbc3ea52fd8cdc357d6be290fcbcd33d05220e36fe7520c1`. Repository snapshots and hashes are saved in [provenance.json](../verification/results/provenance.json). The paper itself says on p.2 that its proof is not a formal proof. Its statements about AI-assisted checking are not evidence of proof-assistant verification.

## What the paper proposes

Theorem 1.1 claims an O(n log n) algorithm for four-coloring a planar graph. The structural advance is Theorem 3.3: find, in linear time, either linearly many pairwise non-touching induced D-reducible configurations or linearly many suitably separated, non-crossing obstructing cycles.

An obstructing cycle has length 3 or 4 with at least one vertex on each side, or length 5 with at least two vertices on each side. In the selected cycle family the private parts do not touch; public parts can share up to an edge. Merely finding arbitrary short cycles does not supply these guarantees.

The catalogue D has 8,202 elements: 8,200 supplied files plus the degree-3 and degree-4 singleton configurations. The 84 oriented discharging rules come from 43 drawings with reflections, two of which are symmetric. Charge starts at 10(6-degree), with total 120, and is redistributed without changing that total.

Theorem 6.7 supplies local reductions near positive charge, zero-charge high-degree vertices, and sufficiently large flat neighborhoods. The flat case is essential: concentrating on positive charge alone cannot guarantee a constant-factor reduction. The sole all-degree-6 configuration in the supplied catalogue is `D2834.conf`; the exceptional radius-3 configuration is `D5059.conf`.

After deleting non-touching configurations, the algorithm triangulates, recursively colors the smaller graph, removes auxiliary edges, and restores the deleted regions. D-reducibility permits repeated Kempe changes even after other regions have been restored. Section 12 coordinates these changes using conditional expectations. Section 13 gives a different reduction and reconstruction scheme for short separating cycles.

The intended recurrence is T(n) ≤ T(αn) + O(n log n) for a fixed α<1, or the analogous sum over smaller components with total size at most αn. Summing the geometric sizes gives O(n log n). This recurrence calculation is conditional on the structural, restoration, and accounting claims; it does not establish those claims by itself.

## Findings

### F1. Non-touching configurations need not have disjoint rings

**Location:** proof of Lemma 12.1, p.49. **Status:** concrete counterexample; finite graph relations checked in Lean.

The proof asserts that the rings of active non-touching configurations are vertex-disjoint and proposes an array mapping each vertex to one host ring.

Take an octahedron with poles `0,1` and equatorial cycle `2-3-4-5-2`. Each pole is adjacent to every equatorial vertex, and the poles are not adjacent. Let the configurations be the singleton poles. Each is degree 4 and is D-reducible by Lemma 2.1. They are non-touching, but **both rings are exactly `{2,3,4,5}`**. Coloring the equatorial cycle with four distinct colors makes both rings non-extendible without a Kempe change, so the issue applies to the restoration step.

[PaperAudit.lean](../verification/PaperAudit.lean) proves `poles_non_touching`, `poles_same_ring`, `four_ring_vertices`, and `overlapping_rings`. The independent Python check supplies and validates the eight oriented triangular faces. Planarity of the double cone over C4 is a mathematical construction here; it is not itself a Lean theorem in this package.

**Effect:** the stated single-host array can lose a ring and omit its contribution to the conditional-expectation calculation. The assertion cannot serve as the linear-time justification in its present form.

**Proposed repair:** store all vertex-to-ring incidences, or build chain-to-ring incidences by scanning every ring after labeling Kempe components. Deduplicate each ring's component IDs. Since each ring has at most 18 positions, total incidence count is at most 18r even when many rings share a vertex. The total pass then costs O(n + sum |R_i|), which is O(n), because the configurations are disjoint and r≤n. An individual component can require more than O(|X|) work; use the total incidence bound instead. Handle repeated positions of a single ring as well as overlap between different rings.

Keep conditional-expectation weights exactly, for example with integers `2^(18-u_i)`. The local identity that one of the two choices preserves the potential is proved in Lean; the complete repaired scheduler and its amortized complexity remain unformalized. A Python example exercises the repair with overlapping incidences.

### F2. Lemma 9.3 overstates what a minimal image preserves

**Location:** §9.2, p.27. **Status:** counterexample checked in Lean, including the relevant definitions.

As printed, Lemma 9.3 says a minimal homomorphic image of a dart representation satisfying M1-M6 also satisfies M1-M6. A homomorphism can merge vertices without merging their incidence lists.

| Structure | Vertices | Darts | Heads | Reverse | Successor/predecessor |
|---|---|---|---|---|---|
| Source | a, b | e, f | e→a, f→b | e↔f | all nil |
| Image | w | e, f | e→w, f→w | e↔f | all nil |

Map a and b to w and keep e and f distinct. The source is a single edge and satisfies all six conditions. The map preserves every required pointer, is surjective on vertices and darts, and satisfies the paper's minimality requirement for nil pointers. The image satisfies M1-M5, but it has two separate one-dart incidence lists at w, violating M6. Loops are allowed for the dart representations in this lemma.

The Lean file verifies `source_basic`, `source_single_list`, `target_basic`, `target_minimal`, and `target_not_single_list`. Its M6 expresses that any two darts with the same head are connected through successor/predecessor steps; under M1-M4 this captures the relevant single-list condition.

**Proposed repair:** restrict the generic preservation statement to M1-M5, and prove M6 for the particular construction where vertex identifications are induced by dart identifications. Lemma 9.6 on p.30 already provides a separate argument of this kind. This makes the error appear locally repairable, but the corrected dependency must be formalized; the general statement is false.

### F3. Algorithm A.1.2 is not executable correctly as printed

**Location:** p.58. **Status:** direct pseudocode inspection, confirmed visually.

The algorithm defines DFS but never calls `DFS(1)` before returning the result set. Also, its terminal branch for `i>n` records a coloring but does not return, allowing the following loop to refer to an out-of-range vertex. The neighbor loop starts at 0 despite the declared vertex ordering starting at 1. A literal translation can return an empty set or access invalid vertices.

**Repair:** initialize the result set and assignment map separately; call `DFS(1)`; return immediately after recording a complete coloring; use `1≤j<i`; test adjacency in the free completion, including ring edges. Verify against an independent enumerator on tiny completions.

This finding concerns the printed pseudocode. The inspected C++ checker uses a different, dual edge-coloring representation; its `color_dfs` has a terminal return and is invoked by `CheckColorability`. It is not established that the C++ implementation has the same defects. A formal connection between the primal pseudocode and the dual implementation is still required.

### F4. Boolean D-reducibility output is insufficient evidence for the level-25 bound

**Location:** Lemma 3.2(D3), pp.8-9; `OneReduction` in the pinned reducibility checker. **Status:** unresolved proof/certificate obligation, not a counterexample to D3.

Restoration requires a certified level for every ring coloring, not just a Boolean answer that the configuration is D-reducible. The inspected checker updates its feasible set in place. A coloring accepted later in a sweep can depend on one accepted earlier in that same sweep. Therefore the number of sweeps cannot automatically be interpreted as the maximum extendibility level. The routine returns feasibility flags rather than a table of levels and witnesses.

For implementation, generate rank certificates from frozen previous-level sets, or assign and validate explicit dependency ranks. Verify the universal quantifier over compatible exterior Kempe patterns and the existential improving move for each pattern. Prove the translation between the dual edge-coloring data and the paper's vertex-ring colors. The full catalogue's level≤25 claim was not reproduced in this review.

### Other corrections to make before formalization

- **Small graphs:** the degree≥3 argument in Lemma 6.1 needs n≥4. A triangle is a sphere triangulation under the stated definition, has degree 2, receives no rule charge, and has charge 40 per vertex; it satisfies the lemma's numerical bounds but not that intermediate degree assertion. Handle n≤3 explicitly in the algorithm.
- **Accounting, p.17:** the expression bounding the complement of W should retain the zero-charge, degree≥9 vertices in U. They also have bounded degree, so an O(|U|) term is available. This is an omitted term in the displayed derivation, not a counterexample to the intended conclusion.
- **Cycle accounting, p.53:** the final inequality printed as `b≤100b` does not supply the needed estimate; use the earlier `100b≤c`. With that hypothesis, Lean verifies the claimed saving of at least c/2.
- **Quantifiers:** replace the informal “linear or sublinear” accounting discussion with explicit universal constants and thresholds, and spell out the treatment of neighborhoods containing degree-3/4 vertices before invoking Theorem 6.7's minimum-degree hypothesis.

## Results and trust boundaries

### Proof-assistant checks

Lean **4.19.0** successfully compiled the file and audited the axioms of 17 named results. There are no proof holes, additional axioms, or uses of `native_decide`. Some results use Lean's standard `propext` and `Quot.sound`; the general Kempe preservation proof does not use axioms. See [lean.log](../verification/results/lean.log).

The proved positive results are Euler-charge arithmetic **assuming** the edge and handshake identities, the charge upper bound **assuming** the incoming-charge bound, the degree accounting inequality, corrected cycle savings, the binary conditional-expectation choice, and preservation of a proper coloring under a swap on a union of bichromatic components. The last theorem works for arbitrary vertex types and edge relations, under an explicit component-closure hypothesis. It does not assert that improving moves always exist.

### Independent finite checks

The Python checker passed all 8,200 files for selected D0-D2 properties: internal connectivity, diameter≤4, the length-four boundary-path condition, radius≤2 with one radius-3 exception, high-degree-center restrictions, at most 19 internal vertices, and ring size≤18. It found 1,677 configurations with one cut vertex. Including their two extensions and mirror orientations explains the C++ loader's 19,754 patterns; this is not a discrepancy with 8,200 catalogue entries.

It also enumerated the cycle boundary color classes: C3 has 1, C4 has 4, and C5 has 10 up to permutation of colors. C5 has five classes using three colors. The cardinalities of the replacement-gadget boundary sets in Lemmas 13.1-13.2 agree with enumeration. This checks their finite color-pattern counts, not the Kempe reachability or topological proofs.

Detailed outputs and limitations are in [independent_checks.json](../verification/results/independent_checks.json).

### Reproduced author computations

The pinned C++ sources were compiled with g++ 13.3.0, assertions enabled, header-only fmt 11.2.0 and spdlog 1.15.3, and the installed Boost libraries. No upstream algorithm source was edited. The build bypassed CMake's dependency discovery and GoogleTest download; the upstream unit-test suite was not run.

| Check | Reported target | Observed result | Scope |
|---|---:|---:|---|
| A.1 combined rules | 1,832 | 1,832 | Reproduced |
| A.1 maximum charge | 8 | 8 | Reproduced |
| A.2 non-blocked combined rules | 671 | 671 | Reproduced |
| A.2 maximum charge | 5 | 5 | Reproduced |
| Initial wheels, degree 7 | 5,439 | 5,439 | Reproduced in 30.6 s |
| Initial wheels, degree 8 | 6,790 | 6,790 | Reproduced in 41.6 s |
| Initial wheels, degree 9 | 3,285 | 3,285 | Reproduced in 108.1 s |
| Initial wheels, degree 10 | 626 | No completed output | Timed out at 180 s |
| Initial wheels, degree 11 | 8 | No completed output | Timed out at 180 s |
| D0000, planar D-reducibility | D-reducible | Passed | One sample, ring size 6 |
| D2834, planar D-reducibility | D-reducible | Passed in 17.7 s | Flat sample, ring size 14 |

The initial wheel jobs and two sample reducibility jobs are recorded separately in [wheels_summary.json](../verification/results/wheels_summary.json) and [sample summary](../verification/results/reducibility_samples/summary.json). Timings are observations from this machine, with other review work running, not controlled benchmarks. Initial wheel enumeration is only a precursor to A.3. Sample reducibility success does not establish the entire catalogue or its level bound. The sample checker was built with an additional `-include fmt/ranges.h` to provide the `fmt::join` declaration; no upstream source was edited.

The complete A.3 bad-cartwheel enumeration, A.4-A.6 combined-cartwheel checks, all 8,200 D-reducibility checks, and an end-to-end graph-coloring implementation were **not** verified. Matching counts from the authors' implementation is reproduction, not an independent proof that its enumeration is exhaustive. The C++ runtime, compiler, input parsing, mathematical interpretation, and completeness of pruning remain outside the Lean trust boundary.

## Practical consequences for a demo

The existence proof gives very weak numerical worst-case guarantees: the per-pass improvement fraction is at least 1/786,432, and the 25-pass block guarantee is approximately **4.06×10^-148**. The flat-region packing bound used in the proof is **8·5^25 = 2,384,185,791,015,625,000**. These constants do not predict measured performance, but they prevent treating the asymptotic theorem as a promise of an efficient literal prototype.

Build an instrumented educational demonstrator first, with explicit limits and independent output validation. Include genuine catalogue configurations and Kempe restoration, and make unsupported cases visible. A research implementation claiming the full algorithm needs both reduction branches, complete certificates, corrected incidence accounting, and a complexity audit. The companion [implementation plan](demo-implementation-plan.md) sets these gates.

## Remaining formalization work

1. Formalize embedded finite graphs, dart rotations, boundaries, and the definition of obstructing cycles; prove the corrected minimal-image and induced-embedding results.
2. Build a small verified certificate checker for D-reducibility, ranks, degree-range coverage, and discharging/cartwheel pruning; feed it versioned certificates generated by an untrusted search program.
3. Verify every catalogue entry and the full unavoidability computation, including the flat case and low-degree handling.
4. Prove constant-factor packing with explicit constants, then verify the corrected shared-ring scheduler, reduction/restoration operations, cycle gadgets, and the recursive cost model.

Until those obligations are discharged, the defensible description is **“partially formalized, with reproduced computational checks and documented corrections.”**

## External references

- [Paper, version 2](https://arxiv.org/abs/2603.24880v2).
- [Authors' repository overview](https://github.com/near-linear-4ct).
- [Pinned computer-check source and execution instructions](https://github.com/near-linear-4ct/computer-checks/tree/6cb85666ce89cf8bd813132e4c5dd56053e367a5).
- [Published reproducibility metrics](https://github.com/near-linear-4ct/instructions-for-checking-reproducibility/tree/82e20d89ba8927521e9e87135c02c85d08f66b18).
- [Pinned reducibility checker](https://github.com/edge-coloring/reducibility_checker/tree/54726f6fc9611a4cd4a794a1d27dd51e3dcd778d).

Execution instructions and trust limits are also in [verification/README.md](../verification/README.md).
