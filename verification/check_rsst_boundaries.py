#!/usr/bin/env python3
"""Independent finite checks of RSST 6.3/6.4 and all 6.5 ring augmentations.

This is exhaustive executable evidence, not a Lean proof of the Rust solver.
"""
import itertools
import json
from functools import lru_cache


def canonical(colors):
    renaming = {}
    return tuple(renaming.setdefault(c, len(renaming) + 1) for c in colors)


def xor(colors):
    value = 0
    for c in colors:
        value ^= c
    return value


@lru_cache(None)
def matchings(positions):
    if not positions:
        return ((),)
    result = []
    # Pair the first endpoint with an odd-positioned endpoint; both sides
    # must themselves have noncrossing perfect matchings.
    for j in range(1, len(positions), 2):
        for left in matchings(positions[1:j]):
            for right in matchings(positions[j+1:]):
                result.append(((positions[0], positions[j]),) + left + right)
    return tuple(result)


def compatible_orbits(pattern, omitted, matching):
    result = set()
    for flips in itertools.product([False, True], repeat=len(matching)):
        p = list(pattern)
        for flip, (a, b) in zip(flips, matching):
            if flip:
                p[a] ^= omitted
                p[b] ^= omitted
        result.add(canonical(p))
    return result


def pair(i, j):
    p = [1] * 5
    p[i], p[j] = 2, 3
    return canonical(p)


c4 = [canonical(p) for p in [(1, 1, 1, 1), (1, 2, 2, 1),
                             (1, 2, 1, 2), (1, 1, 2, 2)]]
families4 = [{c4[i], c4[(i+1) % 4]} for i in range(4)]
families_c = [{pair(i, (i-1) % 5), pair(i, (i+1) % 5),
               pair((i-1) % 5, (i+1) % 5)} for i in range(5)]
families_d = [{pair((i+a) % 5, (i+b) % 5)
               for a in [1, 2] for b in [3, 4]} for i in range(5)]
family_e = {pair(i, (i+1) % 5) for i in range(5)}
counts = {}
for n in [4, 5]:
    all_patterns = {canonical(p) for p in itertools.product([1, 2, 3], repeat=n)}
    patterns = sorted(p for p in all_patterns if xor(p) == 0)
    # Every excluded orbit fails the even-endpoint requirement for some
    # omitted color, so no consistent set can contain it.
    for p in all_patterns - set(patterns):
        assert any(sum(c != omitted for c in p) % 2 for omitted in [1, 2, 3])
    witnesses = {}
    for p in patterns:
        for omitted in [1, 2, 3]:
            pos = tuple(i for i, c in enumerate(p) if c != omitted)
            witnesses[p, omitted] = [compatible_orbits(p, omitted, m) for m in matchings(pos)]
    consistent = 0
    for bits in range(1, 1 << len(patterns)):
        chosen = {p for i, p in enumerate(patterns) if bits & (1 << i)}
        if not all(any(w <= chosen for w in witnesses[p, omitted])
                   for p in chosen for omitted in [1, 2, 3]):
            continue
        consistent += 1
        if n == 4:
            assert any(f <= chosen for f in families4)
        elif chosen & family_e:
            assert any(f <= chosen for f in families_c + families_d + [family_e])
    counts[str(n)] = {'boundary_orbits': len(patterns),
                      'subsets_examined': (1 << len(patterns)) - 1,
                      'nonempty_consistent_sets': consistent}

augmentations = 0
for n in [4, 5]:
    for shift in range(n):
        kinds = range(2 if n == 4 else 3)
        for kind in kinds:
            observed = set()
            for colors in itertools.product(range(4), repeat=n):
                c = [colors[(i+shift) % n] for i in range(n)]
                if any(c[i] == c[(i+1) % n] for i in range(n)):
                    continue
                if n == 4:
                    allowed = c[1] == c[3] if kind == 0 else c[0] != c[2]
                else:
                    allowed = (c[2] == c[4] if kind == 0 else
                               c[3] != c[0] and c[3] != c[1] if kind == 1 else
                               len(set(c)) <= 3)
                if allowed:
                    observed.add(canonical([c[i] ^ c[(i+1) % n] for i in range(n)]))
            expected = ([{c4[0], c4[1]}, {c4[1], c4[2]}][kind] if n == 4
                        else [families_c[0], families_d[0], family_e][kind])
            assert observed == expected, (n, shift, kind)
            augmentations += 1
printed = set()
for c in itertools.product(range(4), repeat=5):
    if any(c[i] == c[(i+1) % 5] for i in range(5)):
        continue
    if c[1] != c[3] and c[1] != c[4]:
        printed.add(canonical([c[i] ^ c[(i+1) % 5] for i in range(5)]))
assert printed == families_d[3] and printed != families_d[0]
counterexample = [0, 1, 2, 0, 2]
counterexample_edges = canonical([counterexample[i] ^ counterexample[(i+1) % 5]
                                 for i in range(5)])
assert counterexample_edges in printed and counterexample_edges not in families_d[0]
print(json.dumps({'lemmas_6_3_and_6_4': counts,
                  'augmentation_families_checked': augmentations,
                  'paper_D1_index_correction': {'printed_diagonals': ['v2-v4', 'v2-v5'], 'printed_family': 'D4', 'corrected_diagonals': ['v4-v1', 'v4-v2'], 'corrected_family': 'D1', 'counterexample_vertex_colors': [0,1,2,0,2]},
                  'formal_verification_of_rust': False}, indent=2))
