# Four-Color Reduction Lab: implementation backlog

This is the build order for the [implementation plan](demo-implementation-plan.md). It uses the corrections and trust boundaries in the [verification report](verification-report.md). It is a plan for future implementation; the existing Lean and computational checks do not certify a demo application.

## Release decision

Build a local educational application first. A successful session loads a validated sphere triangulation, displays an actual reduction, colors the smaller graph with an explicitly labeled bounded solver, restores the original graph, and independently checks its final coloring. The user can replay and export the computation.

The minimum release needs the tetrahedron and octahedron. The octahedron must demonstrate two non-touching degree-4 configurations with the same ring, including a non-extendible boundary coloring followed by a valid Kempe change. This is the first acceptance gate because it exercises the paper's most consequential implementation correction.

After that gate, add D0000, the flat configuration D2834, and separating-cycle examples. A concrete extension witness certifies one run. It must never be presented as a universal D-reducibility or level-25 certificate. Missing catalogue rank certificates therefore do not block the degree-3/4 release.

Excluded from the first release: arbitrary planar graph import without an embedding, the complete local-unavoidability search, all 8,200 catalogue entries, general nested separator recursion, a proof of O(n log n), and a fully formalized theorem.

## Components and interfaces

| Component | Implementation choice | Responsibility |
|---|---|---|
| Browser interface | TypeScript, HTML/CSS, SVG initially | Graph inspection, accessible color labels, event playback, import/export |
| Mathematical engine | C++20 command-line executable | Input validation, matching, reversible transformations, bounded search, trace generation |
| Independent checker | Python standard library initially | Validate original-edge coloring certificates and supported trace operations without calling engine validation code |
| Local runner | Python localhost service, after CLI stabilization | Launch engine with size/time limits and cancellation; serve the interface |
| Formal evidence | Existing Lean 4.19 project plus new modules | Prove narrowly stated checker and transformation properties as their definitions stabilize |
| Catalogue evidence | Pinned source files, hashes, generated witnesses | Reproducible provenance and explicit certificate scope |

Keep the engine executable usable without the browser. The first UI can replay trace files; interactive computation comes after CLI correctness. WebAssembly, Canvas rendering, and deployment are later decisions, not prerequisites.

### Input contract: `EmbeddedGraphV1`

- `schemaVersion`, stable graph ID, and optional title.
- Vertices with unique integer IDs; optional coordinates are display data only.
- Undirected edges, with no loops or duplicate endpoint pairs.
- Consistently oriented triangular faces, from which rotations and reverse darts are derived.
- Optional example hints: nominated configuration interiors, catalogue source IDs, or a nominated cycle and interior. Hints must be validated before use.

For the first release, accept connected sphere triangulations with 3-200 vertices. Handle the triangle as an explicit base case. The 200-vertex cap is an initial input limit, not a performance promise. Validate all IDs and cardinalities before indexing arrays. Check opposite edge incidences, single-cycle vertex links, connectedness, and Euler characteristic; do not accept an embedding solely because Euler's formula holds.

### Result and trace contracts

Use distinct results: `complete`, `unsupported`, `budget_exceeded`, `invalid_input`, and `internal_error`. Only `complete` may display a successful coloring result, and only after the independent checker accepts it. A search timeout conveys no mathematical impossibility.

Each trace contains a schema version, engine version, canonical input hash, source/certificate hashes where used, declared computation budgets, solver provenance, and ordered events. Each event has an ID, type, explanation, affected IDs, a state snapshot or reversible delta, and the witnesses required by its checker. Do not import executable code or render imported explanations as HTML.

Begin with snapshots for the small curated examples. Establish a trace-size limit and reject oversized imports. Convert to checkpoints and deltas only after replay correctness is established and profiling shows a need.

The minimum event sequence is:

```text
InputValidated → BatchSelected → Reduced → SubproblemColored
  → OriginalTopologyRestored → [KempeComponentsBuilt → SwapChosen]*
  → ConfigurationExtended* → ColoringValidated
```

Later events include `ChargeTransferred`, `BoundaryClassesChecked`, `CycleReplaced`, and `ComponentRestored`. Stop events retain the last coherent state and explain the missing capability or exhausted budget.

## Ordered work packages

| ID | Deliverable and files | Depends on | Acceptance test |
|---|---|---|---|
| P0 | Corrected specification, `schemas/`, evidence manifest | Existing review | Shared rings are represented as lists of incidences; generic minimal-image preservation does not assume M6; coloring enumeration has a reachable base case and consistent indices |
| P1 | `engine/embedding.*`, `examples/`, input validator | P0 | Triangle, tetrahedron, and octahedron pass; missing/reversed faces, loops, duplicate edges, bad IDs, disconnected and non-spherical embeddings fail cleanly |
| P2 | `engine/trace.*`, `tests/check_trace.py`, CLI protocol | P1 | Deterministic trace replay reproduces every state; corrupted colors and omitted original edges are rejected by a separate checker |
| P3 | `engine/reduction.*`, degree-3/4 restoration | P2 | Non-touching interiors are checked; auxiliary edges are journaled and removed; every original vertex and edge returns; final coloring passes independently |
| P4 | `engine/kempe.*`, incidence lists, exact scheduler | P3 | Octahedron records 8 ring incidences and 4 unique ring vertices; both rings survive lookup; swaps cover whole bichromatic components; exact conditional-expectation identities hold |
| P5 | `app/` graph view and trace player | P2; P4 for full lesson | Next/previous/reset reproduce checked states; shared membership remains inspectable; vertex color numbers remain readable without distinguishing hues |
| P6 | Local runner and explicit search controls | P3, P5 | Imported data is bounded and validated; cancel/time limit terminates computation; search use is visible in the UI and exported trace |
| P7 | `engine/catalogue.*`, D0000 witness support | P3, P4 | Pinned configuration matches degrees, orientation and inducedness; instance extension witnesses validate; unsupported restoration stops explicitly |
| P8 | `engine/discharging.*`, D2834 closed example | P7 | All 84 oriented rule files are processed; initial and final total charge is 120; D2834's internal degrees are all 6 after closing the patch |
| P9 | `engine/separators.*`, selected 3/4/5-cycle examples | P2, P6 | Boundary class counts are 1/4/10; selected gadget permits only certified extendible classes; restored coloring agrees on the entire boundary and every original edge |
| P10 | Release fixtures, documentation, profiling | P4-P9, narrowed if necessary | All supported examples complete; negative cases stop truthfully; exported traces recheck independently; launch instructions work from a clean build |

Implement P0-P4 before broadening mathematical scope. P5 can use fixed engine-generated traces once P2 is stable. Release the narrower P0-P6 demonstration if P7's certificate work is not ready; state the reduced scope in the release notes.

### P3-P4: first mathematical slice

1. Construct the octahedron with poles 0 and 1 and equatorial cycle 2-3-4-5-2. Validate its eight oriented faces.
2. Select the two singleton poles. Record separate configuration identities and a ring membership list containing both identities at every equatorial vertex.
3. Remove both poles. Add the required auxiliary diagonals in the two exposed faces, recording the side and origin of each edge.
4. Color the reduced graph. Record `bounded-search` provenance and a deterministic search-node count.
5. Remove the auxiliary edges, restoring the original topology with the poles still uncolored. Verify the ring's four-distinct-color case requires recoloring.
6. Label complementary two-color components. Build component-to-ring incidences and deduplicate repeated component IDs within each ring.
7. Obtain a valid degree-4 improving prescription. Use integer weights such as `2^(18-u)` for the undecided incidences; independently verify `potentialNo + potentialYes = 2 × potentialBefore` and the chosen branch's nondecrease.
8. Swap complete components, restore both poles, and verify all 12 original edges. Export the trace and replay it backward and forward.

The executable checker must recompute the component closure condition. A trace's assertion that a set is a Kempe component is not sufficient evidence.

### P7-P9: controlled expansion

For each catalogue example, distinguish three artifacts: a valid embedded occurrence, a concrete coloring-extension witness, and a general ranked restoration certificate. Deliver the first two for the educational application if the third is unavailable. No instance-only result may be labeled as a certified level bound.

For separator examples, start with supplied, validated cycles rather than a global detection algorithm. Check chordlessness, the actual inside/outside partition in the embedding, and the paper's minimum side sizes. A 5-cycle with only one vertex on either side must be rejected as an obstructing-cycle example.

Enumerate small boundary color classes independently and record which classes each replacement permits. Support the two diagonals and two identifications for a 4-cycle, and the five diagonal, five identification, and one star families for a 5-cycle. The selected small example must have concrete extension witnesses covering every permitted class. If it does not, report unsupported; do not invent the paper's missing Kempe reconciliation step. Track merged vertex identities and apply the final color permutation to the whole stored interior coloring.

## Test and verification gates

Use separate tests for mathematical correctness, interface behavior, and formal evidence:

- **Instance correctness:** every original vertex has one color in 0-3; every original edge has different endpoint colors; the immutable original graph is the reference.
- **Transformation correctness:** proper partial coloring at each state; complete Kempe components; reversible auxiliary topology; no lost shared-ring incidences; complete boundary reconciliation.
- **Negative cases:** touching interiors, malformed faces, repeated ring positions, duplicate component incidences, unsupported catalogue entries, a one-sided 5-cycle, corrupted witnesses, exhausted budgets, and oversized JSON.
- **Metamorphic cases:** rename vertices, permute colors, and reverse all face orientations; accepted equivalent inputs retain valid results. Compare small-instance solver results with an independent exhaustive oracle.
- **Interface behavior:** keyboard operation, play/pause/step/reset, example switching during computation, import/export, error states, and narrow-screen layout.
- **Formal evidence:** preserve the existing Lean checks; separately prove each added checker sound against an explicit mathematical specification. Passing Python/C++ tests does not extend the Lean theorem automatically.

The complete level-certificate checker, universal exterior-pattern coverage, full unavoidability computation, corrected packing argument, and recursive complexity proof remain separate research gates. They are not prerequisites for honestly labeled instance demonstrations.

## Schedule and completion definition

Suggested allocation for one engineer familiar with graph algorithms:

| Period | Target |
|---|---|
| Week 1 | P0-P2: contracts, embedding validation, independent certificate checker |
| Weeks 2-3 | P3-P6: degree-3/4 reduction, shared-ring Kempe restoration, playable local demo |
| Weeks 4-5 | P7-P8: small catalogue instances, discharging, closed flat example |
| Weeks 6-8 | P9-P10: separator examples, adversarial testing, documentation and release |

These are planning ranges, not measured delivery promises. Re-estimate after the octahedron slice. If catalogue or separator certificate generation exceeds its budget, ship the narrower verified-instance scope and record the remaining work.

The educational release is complete when supported examples produce genuine replayable computations, an independent checker accepts their colorings, unsupported inputs receive explicit outcomes, all search substitutions are visible, and another developer can build and run the application from the instructions. A polished animation alone does not satisfy this gate.

Record vertices/edges processed, search nodes, selected configurations, ring incidences, Kempe passes, reduction ratios, elapsed time, and peak memory. Report bounded search, rendering, trace serialization, and preprocessing separately. Do not infer O(n log n) from these measurements.
