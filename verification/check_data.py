#!/usr/bin/env python3
"""Independent finite checks; these are executable evidence, not Lean proofs."""
from collections import Counter, deque
from fractions import Fraction
from itertools import product
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent


def distances(adj, start):
    dist = {start: 0}
    q = deque([start])
    while q:
        u = q.popleft()
        for v in adj[u]:
            if v not in dist:
                dist[v] = dist[u] + 1
                q.append(v)
    return dist


def read_configuration(path):
    rows = [list(map(int, line.split())) for line in path.read_text().splitlines() if line.strip()]
    n, ring = rows[0]
    rot = {row[0]: row[2:] for row in rows[1:]}
    degree = {row[0]: row[1] for row in rows[1:]}
    vertices = set(range(ring + 1, n + 1))
    assert set(rot) == vertices and len(rot) == n - ring
    for u, neighbors in rot.items():
        assert degree[u] == len(neighbors) == len(set(neighbors))
        assert all(1 <= v <= n and v != u for v in neighbors)
        assert all(u in rot[v] for v in neighbors if v in vertices)
    adj = {u: set(rot[u]) & vertices for u in vertices}
    return n, ring, degree, rot, adj


def configuration_checks():
    rows = []
    manifest = {}
    for path in sorted((HERE / 'upstream/reducible-configurations/D').glob('*.conf')):
        n, ring, degree, rot, adj = read_configuration(path)
        vertices = set(adj)
        ds = {u: distances(adj, u) for u in vertices}
        assert all(set(d) == vertices for d in ds.values()), path
        ecc = {u: max(d.values()) for u, d in ds.items()}
        radius, diameter = min(ecc.values()), max(ecc.values())
        assert diameter <= 4 and len(vertices) <= 19 and ring <= 18, path
        assert min(degree.values()) >= 5 and max(degree.values()) <= 12, path
        high = [u for u in vertices if degree[u] > 8]
        assert len(high) <= 1 and (not high or ecc[high[0]] <= 2), path
        cut = set()
        for u in vertices:
            rest = vertices - {u}
            if rest:
                reduced = {v: adj[v] - {u} for v in rest}
                if len(distances(reduced, min(rest))) < len(rest):
                    cut.add(u)
        assert len(cut) <= 1, path
        assert all(degree[u] - len(adj[u]) == 2 for u in cut), path
        # Recover the actual outer facial walk. At a cut vertex, merely testing
        # whether every edge is a boundary edge would be too restrictive.
        zrot = {u: [v for v in rot[u] if v in vertices] for u in vertices}
        unseen = {(u, v) for u in vertices for v in adj[u]}
        outer_walks = []
        while unseen:
            start = min(unseen)
            dart = start
            walk = []
            has_external_gap = False
            while True:
                unseen.remove(dart)
                u, v = dart
                walk.append(u)
                full = rot[v]
                if full[(full.index(u) + 1) % len(full)] not in vertices:
                    has_external_gap = True
                ns = zrot[v]
                dart = (v, ns[(ns.index(u) + 1) % len(ns)])
                if dart == start:
                    break
            if has_external_gap:
                outer_walks.append(walk)
            else:
                assert len(walk) == 3, path
        assert len(outer_walks) == 1, path
        outer = outer_walks[0]
        def on_boundary(path):
            return any(all(outer[(i + j) % len(outer)] == p[j] for j in range(len(p)))
                       for p in (path, path[::-1]) for i in range(len(outer)))
        # Test every shortest length-four path until the condition in 3.1(ii) has a witness.
        def good_path(start, finish):
            def walk(path):
                u = path[-1]
                if u == finish:
                    return (not on_boundary(path)
                            or any(v not in cut and degree[v] - len(adj[v]) >= 2 for v in path[1:-1]))
                return any(walk(path + [v]) for v in adj[u]
                           if ds[start][v] == len(path) and ds[start][v] + ds[v][finish] == 4)
            return walk([start])
        assert all(good_path(u, v) for u in vertices for v in vertices if u < v and ds[u][v] == 4), path
        rows.append({'file': path.name, 'vertices': n - ring, 'ring': ring, 'radius': radius,
                     'diameter': diameter, 'cut_vertices': len(cut),
                     'all_degree_six': all(d == 6 for d in degree.values())})
        manifest[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
    assert len(rows) == 8200
    exceptions = [r for r in rows if r['radius'] > 2]
    assert len(exceptions) == 1 and exceptions[0]['radius'] == 3
    flat = [r for r in rows if r['all_degree_six']]
    assert len(flat) == 1
    (HERE / 'results/configuration_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return {'count': len(rows), 'max_vertices': max(r['vertices'] for r in rows),
            'max_ring': max(r['ring'] for r in rows), 'max_diameter': max(r['diameter'] for r in rows),
            'radius_three': exceptions, 'all_degree_six': flat,
            'cut_vertex_configurations': sum(r['cut_vertices'] for r in rows),
            'checked': ['format', 'internal connectivity', 'D0 path condition', 'D1 radius/high-degree bounds', 'D2 size bounds'],
            'not_checked': ['D3 reducibility/level', 'full free-completion embedding validity', 'Steinberger subset containment']}


def canonical(coloring):
    labels = {}
    return tuple(labels.setdefault(c, len(labels)) for c in coloring)


def cycle_checks():
    result = {}
    for n in (3, 4, 5):
        labeled = [c for c in product(range(4), repeat=n) if all(c[i] != c[(i + 1) % n] for i in range(n))]
        classes = sorted({canonical(c) for c in labeled})
        result[n] = {'labeled': len(labeled), 'up_to_color_permutation': len(classes),
                     'three_or_fewer_colors': sum(len(set(c)) <= 3 for c in classes)}
        if n == 4:
            assert len(classes) == 4
            assert all(sum(c[j] == c[(j + 2) % n] for c in classes) == 2 for j in range(2))
        if n == 5:
            assert len(classes) == 10
            assert all(sum(c.count(c[j]) == 1 for c in classes) == 4 for j in range(5))
            assert all(sum(c[j] == c[(j + 2) % n] for c in classes) == 3 for j in range(5))
            assert sum(len(set(c)) == 3 for c in classes) == 5
    return result


def octahedron_check():
    edges = {tuple(sorted((p, v))) for p in (0, 1) for v in range(2, 6)}
    edges |= {(2, 3), (3, 4), (4, 5), (2, 5)}
    # Oriented faces of the sphere obtained by gluing two disks along C4.
    faces = []
    for a, b in [(2, 3), (3, 4), (4, 5), (5, 2)]:
        faces.extend([(0, a, b), (1, b, a)])
    darts = Counter((f[i], f[(i + 1) % 3]) for f in faces for i in range(3))
    assert len(darts) == 24 and all(count == 1 for count in darts.values())
    assert all((b, a) in darts for a, b in darts)
    assert 6 - len(edges) + len(faces) == 2
    adj = {v: {b if v == a else a for a, b in edges if v in (a, b)} for v in range(6)}
    assert adj[0] == adj[1] == {2, 3, 4, 5} and 1 not in adj[0]
    return {'vertices': 6, 'edges': sorted(edges), 'oriented_faces': faces,
            'non_touching_configurations': [[0], [1]], 'rings': [sorted(adj[0]), sorted(adj[1])]}


def conditional_expectation_check():
    # Deliberately overlapping ring-chain incidences; exact rational potential.
    constraints = [{0: 0, 1: 1}, {0: 1}, {0: 1, 2: 0}, {1: 1, 2: 1}, {0: 0, 1: 1, 2: 1}]
    live = list(constraints)
    potential = sum((Fraction(1, 2 ** len(c)) for c in live), Fraction())
    initial = potential
    decisions = {}
    for chain in (0, 1, 2):
        branches = []
        for bit in (0, 1):
            remaining = [{k: v for k, v in c.items() if k != chain} for c in live if chain not in c or c[chain] == bit]
            value = sum((Fraction(1, 2 ** len(c)) for c in remaining), Fraction())
            branches.append((value, remaining))
        assert branches[0][0] + branches[1][0] == 2 * potential
        bit = int(branches[1][0] > branches[0][0])
        potential, live = branches[bit]
        decisions[chain] = bit
    assert len(live) >= initial
    return {'initial_potential': str(initial), 'survivors': len(live), 'decisions': decisions}


def main():
    result = {'configurations': configuration_checks(), 'cycle_colorings': cycle_checks(),
              'octahedron_counterexample': octahedron_check(),
              'overlapping_incidence_repair_example': conditional_expectation_check(),
              'constants': {'one_step_denominator': 3 * 2 ** 18,
                            'block_success_probability': str(Fraction(1, (3 * 2 ** 18) ** 25)),
                            'block_success_probability_approx': (3 * 2 ** 18) ** -25,
                            'flat_packing_bound': 8 * 5 ** 25}}
    (HERE / 'results/independent_checks.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
