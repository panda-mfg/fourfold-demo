//! Constructive RSST research implementation, based on sections 3 and 6.
//! The catalogue locator is exhaustive, rather than the paper's discharging
//! locator. Consequently this implementation does NOT claim its O(n²) bound.
pub mod plane;
use plane::Plane;
use std::collections::{HashMap, HashSet, VecDeque};
use std::sync::OnceLock;

type Result<T> = std::result::Result<T, String>;
#[derive(Clone)]
struct Configuration {
    name: String,
    graph: Plane,
    ring: usize,
    contract: Vec<usize>,
}
fn parse_catalogue(text: &str) -> Result<Vec<Configuration>> {
    let mut words = text.split_whitespace();
    fn number<'a>(words: &mut impl Iterator<Item = &'a str>) -> Result<usize> {
        words
            .next()
            .ok_or("Truncated catalogue.")?
            .parse()
            .map_err(|_| "Invalid catalogue number.".into())
    }
    let mut configs = Vec::new();
    while let Some(name) = words.next() {
        let n = number(&mut words)?;
        let ring = number(&mut words)?;
        let _extendible = number(&mut words)?;
        let _consistent = number(&mut words)?;
        if !(3..=26).contains(&n) || !(2..=14).contains(&ring) || ring >= n {
            return Err("Catalogue dimensions out of range.".into());
        }
        let k = number(&mut words)?;
        let mut pairs = Vec::new();
        for _ in 0..k {
            pairs.push([number(&mut words)? - 1, number(&mut words)? - 1]);
        }
        let mut rows = Vec::new();
        for i in 0..n {
            if number(&mut words)? != i + 1 {
                return Err("Unexpected catalogue vertex ID.".into());
            }
            let degree = number(&mut words)?;
            let mut row = Vec::new();
            for _ in 0..degree {
                row.push(number(&mut words)? - 1);
            }
            rows.push(row);
        }
        for _ in 0..n {
            number(&mut words)?;
        }
        let graph = Plane::from_neighbors(&rows)?;
        let contract = pairs
            .iter()
            .map(|p| {
                graph
                    .edge(p[0], p[1])
                    .ok_or("Missing contract edge.".into())
            })
            .collect::<Result<Vec<_>>>()?;
        let faces = graph.faces()?;
        if faces.iter().filter(|f| f.len() != 3).count() != 1
            || faces.iter().all(|f| f.len() != ring)
        {
            return Err("Invalid free-completion faces.".into());
        }
        configs.push(Configuration {
            name: name.into(),
            graph,
            ring,
            contract,
        });
    }
    if configs.len() != 633 {
        return Err("RSST requires exactly 633 configurations.".into());
    }
    Ok(configs)
}
fn catalogue() -> &'static [Configuration] {
    static DATA: OnceLock<Vec<Configuration>> = OnceLock::new();
    DATA.get_or_init(|| {
        parse_catalogue(include_str!("../../data/rsst-unavoidable.conf"))
            .expect("Bundled RSST catalogue is validated by tests")
    })
}

#[derive(Default, Clone, Debug)]
pub struct Counters {
    pub configurations: usize,
    pub d_reductions: usize,
    pub c_reductions: usize,
    pub separators: [usize; 4],
    pub boundary_states: usize,
    pub extension_attempts: usize,
    pub matches_tested: usize,
    pub max_depth: usize,
    pub triangulation_vertices: usize,
    pub low_degree_reductions: usize,
    pub configuration_names: Vec<String>,
}
struct Context<'a> {
    run: &'a mut super::Run,
    counts: Counters,
    origins: Vec<Option<usize>>,
    live_bytes: usize,
}
impl Context<'_> {
    fn tick(&mut self) -> Result<()> {
        self.run.tick().map_err(|e| format!("{e:?}"))
    }
    fn check(&mut self) -> Result<()> {
        self.run.check().map_err(|e| format!("{e:?}"))
    }
    fn publish(&mut self, colors: &[u8]) {
        if !self.run.reporting {
            return;
        }
        self.run.colors.fill(-1);
        for (u, &origin) in self.origins.iter().enumerate() {
            if let Some(v) = origin {
                if v < self.run.colors.len() {
                    self.run.colors[v] = colors[u] as i8;
                }
            }
        }
        self.run.phase = 4;
    }
}

fn descend(g: &Plane, map: &[usize], ctx: &mut Context, depth: usize) -> Result<Vec<u8>> {
    let saved = std::mem::replace(&mut ctx.origins, vec![None; g.n()]);
    for (u, &v) in map.iter().enumerate() {
        if v != usize::MAX && ctx.origins[v].is_none() {
            ctx.origins[v] = saved[u];
        }
    }
    let result = solve_t(g, ctx, depth);
    ctx.origins = saved;
    result
}

/// Canonicalize up to permutation of the three edge colors. Unused colors
/// are mapped too, so the entire stored coloring stays a proper tri-coloring.
fn canonical(colors: &mut [u8], ring: &[usize]) -> u32 {
    let mut map = [0u8; 4];
    let mut next = 1;
    for &e in ring {
        let c = colors[e] as usize;
        if map[c] == 0 {
            map[c] = next;
            next += 1;
        }
    }
    for c in 1..=3 {
        if map[c] == 0 {
            map[c] = next;
            next += 1;
        }
    }
    for c in colors.iter_mut() {
        *c = map[*c as usize];
    }
    ring.iter()
        .fold(0, |key, &e| key * 3 + colors[e] as u32 - 1)
}

/// Section 6.2: construct a consistent boundary set using the actual ribs
/// from section 3.1. A state is retained for each boundary-color orbit.
/// All subsets of boundary ribs are generated from each retained state, so
/// discarding duplicate boundaries does not assume Kempe connectivity.
fn boundary_closure<F>(
    g: &Plane,
    ring: &[usize],
    initial: Vec<u8>,
    ctx: &mut Context,
    mut accept: F,
) -> Result<Option<Vec<u8>>>
where
    F: FnMut(&[u8], &mut Context) -> Result<bool>,
{
    if initial.len() != g.ends.len()
        || initial.iter().any(|c| !(1..=3).contains(c))
        || !(2..=14).contains(&ring.len())
        || ring.iter().any(|&e| e >= g.ends.len())
    {
        return Err("Invalid bounded-ring tri-coloring.".into());
    }
    let exterior = g.outer_face(ring)?;
    let exterior_set: HashSet<usize> = exterior.iter().copied().collect();
    let faces: Vec<_> = g
        .faces()?
        .into_iter()
        .filter(|f| !exterior_set.contains(&f[0]))
        .collect();
    if faces.iter().any(|f| f.len() != 3) {
        return Err("Nontriangular finite face in boundary closure.".into());
    }
    let mut first = initial;
    let key = canonical(&mut first, ring);
    let mut seen = HashSet::from([key]);
    let mut queue = VecDeque::from([first]);
    while let Some(state) = queue.pop_front() {
        ctx.check()?;
        ctx.counts.boundary_states += 1;
        if accept(&state, ctx)? {
            return Ok(Some(state));
        }
        for omitted in 1..=3 {
            let mut adj = vec![Vec::new(); g.ends.len()];
            for f in &faces {
                ctx.tick()?;
                let pair: Vec<_> = f
                    .iter()
                    .map(|d| d / 2)
                    .filter(|&e| state[e] != omitted)
                    .collect();
                if pair.len() != 2 || state[pair[0]] == state[pair[1]] {
                    return Err("Invalid finite-face coloring.".into());
                }
                adj[pair[0]].push(pair[1]);
                adj[pair[1]].push(pair[0]);
            }
            let mut visited = vec![false; g.ends.len()];
            let mut ribs = Vec::new();
            for &e in ring {
                if state[e] == omitted || visited[e] {
                    continue;
                }
                let mut rib = vec![e];
                visited[e] = true;
                let mut i = 0;
                while i < rib.len() {
                    let a = rib[i];
                    i += 1;
                    ctx.tick()?;
                    for &b in &adj[a] {
                        if !visited[b] {
                            visited[b] = true;
                            rib.push(b);
                        }
                    }
                }
                let incidence = ring.iter().filter(|e| rib.contains(e)).count();
                if incidence != 2 {
                    return Err("A boundary rib must have exactly two boundary incidences.".into());
                }
                ribs.push(rib);
            }
            if ribs.len() > 7 {
                return Err("RSST boundary exceeds the fourteen-edge limit.".into());
            }
            for mask in 1usize..(1usize << ribs.len()) {
                ctx.tick()?;
                let mut candidate = state.clone();
                for (i, rib) in ribs.iter().enumerate() {
                    if mask & (1 << i) != 0 {
                        for &e in rib {
                            candidate[e] ^= omitted;
                        }
                    }
                }
                let key = canonical(&mut candidate, ring);
                if seen.insert(key) {
                    if (queue.len() + 2).saturating_mul(g.ends.len() + 64) > 64 * 1024 * 1024 {
                        return Err("Resource limit: boundary-state queue exceeded 64 MiB.".into());
                    }
                    ctx.run.stats.swaps += mask.count_ones() as u64;
                    queue.push_back(candidate);
                }
            }
        }
    }
    Ok(None)
}

/// Search only inside a fixed (at most 26 vertex) free completion. This is
/// the constant-sized extension test in 6.5, never a whole-input fallback.
fn extend_small(g: &Plane, colors: &mut [u8], ctx: &mut Context) -> Result<bool> {
    ctx.tick()?;
    let mut best = None;
    let mut best_mask = 0u8;
    let mut size = 5;
    for u in 0..g.n() {
        if colors[u] != 255 {
            continue;
        }
        let mut mask = 15u8;
        for &d in &g.rot[u] {
            let c = colors[g.target(d)];
            if c < 4 {
                mask &= !(1 << c);
            }
        }
        let count = mask.count_ones();
        if count == 0 {
            return Ok(false);
        }
        if count < size {
            best = Some(u);
            best_mask = mask;
            size = count;
        }
    }
    let Some(u) = best else {
        return Ok(true);
    };
    for c in 0..4 {
        if best_mask & (1 << c) == 0 {
            continue;
        }
        colors[u] = c;
        if extend_small(g, colors, ctx)? {
            return Ok(true);
        }
    }
    colors[u] = 255;
    Ok(false)
}

fn match_configuration(
    t: &Plane,
    k: &Configuration,
    anchor: usize,
    offset: usize,
    reverse: bool,
    ctx: &mut Context,
) -> Result<Option<Vec<usize>>> {
    ctx.counts.matches_tested += 1;
    let root = k.ring;
    let n = k.graph.n();
    let mut map = vec![usize::MAX; n];
    map[root] = anchor;
    let mut done = vec![false; n];
    let mut queue = VecDeque::from([(root, offset)]);
    while let Some((u, shift)) = queue.pop_front() {
        if done[u] {
            continue;
        }
        done[u] = true;
        ctx.tick()?;
        let tu = map[u];
        let degree = k.graph.rot[u].len();
        if t.rot[tu].len() != degree {
            return Ok(None);
        }
        for (i, &d) in k.graph.rot[u].iter().enumerate() {
            let v = k.graph.target(d);
            let j = if reverse {
                (shift + degree - i) % degree
            } else {
                (shift + i) % degree
            };
            let tv = t.target(t.rot[tu][j]);
            if map[v] != usize::MAX && map[v] != tv {
                return Ok(None);
            }
            // Only ring vertices may coincide under the projection (3.3).
            if map
                .iter()
                .enumerate()
                .any(|(w, &tw)| w != v && tw == tv && (v >= k.ring || w >= k.ring))
            {
                return Ok(None);
            }
            map[v] = tv;
            if v >= k.ring && !done[v] {
                if t.rot[tv].len() != k.graph.rot[v].len() {
                    return Ok(None);
                }
                let ki = k.graph.rot[v]
                    .iter()
                    .position(|&x| k.graph.target(x) == u)
                    .unwrap();
                let ti = t.rot[tv]
                    .iter()
                    .position(|&x| t.target(x) == tu)
                    .ok_or("Missing reverse incidence.")?;
                let deg = t.rot[tv].len();
                let next = if reverse {
                    (ti + ki) % deg
                } else {
                    (ti + deg - ki) % deg
                };
                queue.push_back((v, next));
            }
        }
    }
    if map.contains(&usize::MAX) {
        return Ok(None);
    }
    // Require an induced occurrence of G(K), including absent internal edges.
    for u in k.ring..n {
        for v in u + 1..n {
            if k.graph.edge(u, v).is_some() != t.edge(map[u], map[v]).is_some() {
                return Ok(None);
            }
        }
    }
    if k.graph
        .ends
        .iter()
        .any(|p| t.edge(map[p[0]], map[p[1]]).is_none())
    {
        return Ok(None);
    }
    // Every finite face must project to a distinct face of T.
    let mut face_ids = HashSet::new();
    for f in k.graph.faces()? {
        if f.len() != 3 {
            continue;
        }
        let a = k.graph.source(f[0]);
        let b = k.graph.source(f[1]);
        let c = k.graph.source(f[2]);
        let Some(d) = t.dart(map[a], map[b]) else {
            return Ok(None);
        };
        let row = &t.rot[map[b]];
        let i = row.iter().position(|&x| x == (d ^ 1)).unwrap();
        let next = row[if reverse {
            (i + 1) % row.len()
        } else {
            (i + row.len() - 1) % row.len()
        }];
        if t.target(next) != map[c] {
            return Ok(None);
        }
        let mut triple = [map[a], map[b], map[c]];
        triple.sort_unstable();
        if !face_ids.insert(triple) {
            return Ok(None);
        }
    }
    Ok(Some(map))
}

fn is_short(t: &Plane, cycle: &[usize]) -> bool {
    let parts = t.components_without(cycle);
    parts.len() >= 2 && (cycle.len() < 5 || parts.iter().all(|p| p.len() >= 2))
}
/// Exhaustive induced short-circuit locator. This is deliberately separate
/// from the paper's linear-time discharging/cartwheel locator.
fn short_circuit(t: &Plane, ctx: &mut Context) -> Result<Option<Vec<usize>>> {
    for u in 0..t.n() {
        let mut seen = HashSet::new();
        for &d in &t.rot[u] {
            let v = t.target(d);
            if !seen.insert(v) {
                return Ok(Some(vec![u, v]));
            }
        }
    }
    fn walk(t: &Plane, path: &mut Vec<usize>, ctx: &mut Context) -> Result<Option<Vec<usize>>> {
        let first = path[0];
        let last = *path.last().unwrap();
        for &d in &t.rot[last] {
            ctx.tick()?;
            let v = t.target(d);
            if v <= first || path.contains(&v) {
                continue;
            }
            if path.len() >= 3
                && path[1..path.len() - 1]
                    .iter()
                    .any(|&u| t.edge(u, v).is_some())
            {
                continue;
            }
            path.push(v);
            let closes = path.len() >= 3 && t.edge(v, first).is_some();
            if closes {
                if path[1] < v && is_short(t, path) {
                    return Ok(Some(path.clone()));
                }
            } else if path.len() < 5 {
                if let Some(c) = walk(t, path, ctx)? {
                    return Ok(Some(c));
                }
            }
            path.pop();
        }
        Ok(None)
    }
    for root in 0..t.n() {
        if let Some(c) = walk(t, &mut vec![root], ctx)? {
            return Ok(Some(c));
        }
    }
    Ok(None)
}

fn ring_edges(g: &Plane, cycle: &[usize]) -> Result<Vec<usize>> {
    (0..cycle.len())
        .map(|i| {
            g.edge(cycle[i], cycle[(i + 1) % cycle.len()])
                .ok_or("Missing ring edge.".into())
        })
        .collect()
}
fn pattern_key(pattern: &[u8]) -> u32 {
    let mut p = pattern.to_vec();
    canonical(&mut p, &(0..pattern.len()).collect::<Vec<_>>())
}
fn pair_pattern(i: usize, j: usize) -> u32 {
    let mut p = vec![1; 5];
    p[i] = 2;
    p[j] = 3;
    pattern_key(&p)
}
fn lift_child(h: &Plane, child: &Plane, child_colors: &[u8], vm: Option<&[usize]>) -> Vec<u8> {
    let _ = child;
    h.ends
        .iter()
        .map(|&[u, v]| child_colors[vm.map_or(u, |m| m[u])] ^ child_colors[vm.map_or(v, |m| m[v])])
        .collect()
}

fn augment_five(
    h: &Plane,
    cycle: &[usize],
    i: usize,
    kind: usize,
) -> Result<(Plane, Option<Vec<usize>>)> {
    let mut second = h.clone();
    let ring = ring_edges(h, cycle)?;
    let face = second.outer_face(&ring)?;
    if kind == 0 {
        let e = second.add_chord(&face, cycle[(i + 2) % 5], cycle[(i + 4) % 5])?;
        let (g, map) = second.contract(&[e])?;
        return Ok((g, Some(map)));
    } else if kind == 1 {
        // D_i is defined by the four edges after e_i in 6.4. The printed
        // D_1 diagonal pair in 6.5 realizes D_4, not D_1. For D_1 the fan
        // must be at v_4: add v_4 v_1 and v_4 v_2. Exhaustively checked in
        // verification/check_rsst_boundaries.py, including all rotations.
        let u = cycle[(i + 3) % 5];
        second.add_chord(&face, u, cycle[i])?;
        let face = second
            .faces()?
            .into_iter()
            .find(|f| f.len() == 4)
            .ok_or("Missing quadrilateral after five-ring chord.")?;
        second.add_chord(&face, u, cycle[(i + 1) % 5])?;
    } else if kind == 2 {
        second.cap(&face)?;
    } else {
        return Err("Invalid five-ring family.".into());
    }
    Ok((second, None))
}

fn solve_split(t: &Plane, cycle: &[usize], ctx: &mut Context, depth: usize) -> Result<Vec<u8>> {
    let size = cycle.len();
    ctx.counts.separators[size - 2] += 1;
    let parts = t.components_without(cycle);
    if parts.len() < 2 || (size >= 3 && parts.len() != 2) {
        return Err("Invalid short-circuit components.".into());
    }
    let mut sides = Vec::new();
    for part in parts {
        let mut keep = vec![false; t.n()];
        for &v in cycle.iter().chain(part.iter()) {
            keep[v] = true;
        }
        let mut edges = vec![true; t.ends.len()];
        if size == 2 {
            let retained = t.edge(cycle[0], cycle[1]).unwrap();
            for (e, &[u, v]) in t.ends.iter().enumerate() {
                if cycle.contains(&u) && cycle.contains(&v) && e != retained {
                    edges[e] = false;
                }
            }
        }
        sides.push(t.restrict(&keep, &edges));
    }
    if size <= 3 {
        let mut result = vec![1; t.ends.len()];
        for (h, vm, em) in sides {
            let vertex = descend(&h, &vm, ctx, depth + 1)?;
            let mut edge = h.edge_colors(&vertex);
            let ring = if size == 2 {
                vec![h.edge(vm[cycle[0]], vm[cycle[1]]).unwrap()]
            } else {
                ring_edges(&h, &cycle.iter().map(|&u| vm[u]).collect::<Vec<_>>())?
            };
            canonical(&mut edge, &ring);
            for (e, &he) in em.iter().enumerate() {
                if he != usize::MAX {
                    result[e] = edge[he];
                }
            }
        }
        return t.vertex_colors(&result);
    }
    let (h1, vm1, em1) = &sides[0];
    let (h2, vm2, em2) = &sides[1];
    let c1: Vec<_> = cycle.iter().map(|&u| vm1[u]).collect();
    let c2: Vec<_> = cycle.iter().map(|&u| vm2[u]).collect();
    let r1 = ring_edges(h1, &c1)?;
    let r2 = ring_edges(h2, &c2)?;
    let mut first = h1.clone();
    let face = first.outer_face(&r1)?;
    if size == 4 {
        first.add_chord(&face, c1[0], c1[2])?;
    } else {
        first.cap(&face)?;
    }
    first.validate(true)?;
    let colors = descend(&first, vm1, ctx, depth + 1)?;
    let initial = lift_child(h1, &first, &colors, None);
    let mut boundary = HashMap::new();
    boundary_closure(h1, &r1, initial, ctx, |edge, _| {
        let mut e = edge.to_vec();
        let key = canonical(&mut e, &r1);
        boundary.insert(key, e);
        Ok(false)
    })?;
    // Choose augmentations matching the families in 6.3--6.4 (see the D_i
    // indexing correction in augment_five). Arbitrary side colorings need
    // not have compatible boundary colorings.
    let mut second = h2.clone();
    let mut identify = None;
    if size == 4 {
        let mut choice = None;
        for shift in 0..4 {
            let key = |p: [u8; 4]| {
                let mut rotated = vec![0; 4];
                for i in 0..4 {
                    rotated[(i + shift) % 4] = p[i];
                }
                pattern_key(&rotated)
            };
            let c0 = key([1, 1, 1, 1]);
            let c1 = key([1, 2, 2, 1]);
            let c2 = key([1, 2, 1, 2]);
            if boundary.contains_key(&c0) && boundary.contains_key(&c1) {
                choice = Some((shift, true));
                break;
            }
            if boundary.contains_key(&c1) && boundary.contains_key(&c2) {
                choice = Some((shift, false));
                break;
            }
        }
        let (shift, contract) = choice.ok_or("Boundary set violates RSST lemma 6.3.")?;
        let face = second.outer_face(&r2)?;
        if contract {
            let e = second.add_chord(&face, c2[(shift + 1) % 4], c2[(shift + 3) % 4])?;
            let (g, map) = second.contract(&[e])?;
            second = g;
            identify = Some(map);
        } else {
            second.add_chord(&face, c2[shift], c2[(shift + 2) % 4])?;
        }
    } else {
        let mut choice = None;
        for i in 0..5 {
            let j = (i + 4) % 5;
            let k = (i + 1) % 5;
            if [(i, j), (i, k), (j, k)]
                .iter()
                .all(|&(a, b)| boundary.contains_key(&pair_pattern(a, b)))
            {
                choice = Some((i, 0));
                break;
            }
            let a = (i + 1) % 5;
            let b = (i + 2) % 5;
            let c = (i + 3) % 5;
            let d = (i + 4) % 5;
            if [(a, c), (a, d), (b, c), (b, d)]
                .iter()
                .all(|&(a, b)| boundary.contains_key(&pair_pattern(a, b)))
            {
                choice = Some((i, 1));
                break;
            }
        }
        if choice.is_none() && (0..5).all(|i| boundary.contains_key(&pair_pattern(i, (i + 1) % 5)))
        {
            choice = Some((0, 2));
        }
        let (i, kind) = choice.ok_or("Boundary set violates RSST lemma 6.4.")?;
        (second, identify) = augment_five(h2, &c2, i, kind)?;
    }
    second.validate(true)?;
    if second.n() >= t.n() {
        return Err("Short-circuit recursion did not reduce input.".into());
    }
    let second_map: Vec<_> = vm2
        .iter()
        .map(|&u| {
            if u == usize::MAX {
                u
            } else {
                identify.as_ref().map_or(u, |m| m[u])
            }
        })
        .collect();
    let colors = descend(&second, &second_map, ctx, depth + 1)?;
    let mut edge2 = lift_child(h2, &second, &colors, identify.as_deref());
    let key = canonical(&mut edge2, &r2);
    let edge1 = boundary
        .get(&key)
        .ok_or("No compatible short-circuit boundary coloring.")?;
    let mut result = vec![0; t.ends.len()];
    for (e, &he) in em1.iter().enumerate() {
        if he != usize::MAX {
            result[e] = edge1[he];
        }
    }
    for (e, &he) in em2.iter().enumerate() {
        if he != usize::MAX {
            result[e] = edge2[he];
        }
    }
    t.vertex_colors(&result)
}

fn solve_reduction(
    t: &Plane,
    k: &Configuration,
    map: &[usize],
    child: Plane,
    vm: Vec<usize>,
    ctx: &mut Context,
    depth: usize,
) -> Result<Vec<u8>> {
    ctx.counts.configurations += 1;
    if k.contract.is_empty() {
        ctx.counts.d_reductions += 1;
    } else {
        ctx.counts.c_reductions += 1;
    }
    if !ctx.counts.configuration_names.contains(&k.name) {
        ctx.counts.configuration_names.push(k.name.clone());
    }
    ctx.run.stats.batches += 1;
    ctx.run.stats.removed += t.n() - child.n();
    let color = descend(&child, &vm, ctx, depth + 1)?;
    let mut keep = vec![true; t.n()];
    for &u in &map[k.ring..] {
        keep[u] = false;
    }
    let (h, hm, _) = t.induced(&keep);
    let hcolors: Vec<_> = (0..t.n())
        .filter(|&u| keep[u])
        .map(|u| color[vm[u]])
        .collect();
    let ring: Vec<_> = (0..k.ring)
        .map(|i| {
            h.edge(hm[map[i]], hm[map[(i + 1) % k.ring]])
                .ok_or("Missing projected ring edge.".into())
        })
        .collect::<Result<_>>()?;
    let mut result = None;
    boundary_closure(&h, &ring, h.edge_colors(&hcolors), ctx, |edge, ctx| {
        ctx.counts.extension_attempts += 1;
        let outer = h.vertex_colors(edge)?;
        let mut local = vec![255; k.graph.n()];
        for i in 0..k.ring {
            local[i] = outer[hm[map[i]]];
        }
        if !extend_small(&k.graph, &mut local, ctx)? {
            return Ok(false);
        }
        let mut answer = vec![255; t.n()];
        for u in 0..t.n() {
            if keep[u] {
                answer[u] = outer[hm[u]];
            }
        }
        for i in k.ring..k.graph.n() {
            answer[map[i]] = local[i];
        }
        if t.ends.iter().any(|p| answer[p[0]] == answer[p[1]]) {
            return Err("Configuration restoration violated an input edge.".into());
        }
        result = Some(answer);
        Ok(true)
    })?;
    result.ok_or_else(|| format!("No extension in consistent boundary set for {}.", k.name))
}

/// Apply the degree-three/four part of the short-circuit reduction iteratively.
/// Opposite nonadjacent neighbors of a degree-four vertex are identified in
/// the plane minor. They receive one color; the removed vertex then sees at
/// most three colors. Compact undo records avoid one full graph per such step.
fn solve_t(t: &Plane, ctx: &mut Context, depth: usize) -> Result<Vec<u8>> {
    let cost = 4
        * (t.n() * std::mem::size_of::<Vec<usize>>()
            + t.rot
                .iter()
                .map(|r| r.capacity() * std::mem::size_of::<usize>())
                .sum::<usize>()
            + t.ends.capacity() * std::mem::size_of::<[usize; 2]>());
    if depth >= 512 || ctx.live_bytes + cost > 256 * 1024 * 1024 {
        return Err(
            "Resource limit: recursion depth 512 or estimated graph storage 256 MiB.".into(),
        );
    }
    ctx.live_bytes += cost;
    let result = solve_t_inner(t, ctx, depth);
    ctx.live_bytes -= cost;
    result
}
fn solve_t_inner(t: &Plane, ctx: &mut Context, depth: usize) -> Result<Vec<u8>> {
    let mut active = t.clone();
    let mut labels: Vec<_> = (0..t.n()).collect();
    let mut undo: Vec<(usize, Vec<usize>, Option<(usize, usize)>)> = Vec::new();
    loop {
        ctx.check()?;
        if active.n() <= 5 {
            break;
        }
        let parallel = active.rot.iter().any(|r| {
            let mut seen = HashSet::new();
            r.iter().any(|&d| !seen.insert(active.target(d)))
        });
        if parallel {
            break;
        }
        let offset = (ctx.run.variant as usize * 997) % active.n();
        let Some(u) = (0..active.n())
            .map(|i| (i + offset) % active.n())
            .find(|&u| active.rot[u].len() <= 4)
        else {
            break;
        };
        let neighbors: Vec<_> = active.rot[u].iter().map(|&d| active.target(d)).collect();
        if neighbors.len() < 3 {
            return Err("Low degree in a simple triangulation.".into());
        }
        let neighbor_labels = neighbors.iter().map(|&v| labels[v]).collect();
        if neighbors.len() == 3 {
            let mut keep = vec![true; active.n()];
            keep[u] = false;
            let next = active.induced(&keep).0;
            undo.push((labels[u], neighbor_labels, None));
            labels.remove(u);
            active = next;
        } else {
            let i = if active.edge(neighbors[0], neighbors[2]).is_none() {
                0
            } else {
                1
            };
            let a = neighbors[i];
            let b = neighbors[i + 2];
            if active.edge(a, b).is_some() {
                return Err("No nonadjacent opposite pair at degree four.".into());
            }
            let (next, vm) =
                active.contract(&[active.edge(u, a).unwrap(), active.edge(u, b).unwrap()])?;
            let mut next_labels = vec![usize::MAX; next.n()];
            for v in 0..active.n() {
                if v != u && v != b {
                    next_labels[vm[v]] = labels[v];
                }
            }
            undo.push((labels[u], neighbor_labels, Some((labels[a], labels[b]))));
            labels = next_labels;
            active = next;
        }
        ctx.counts.low_degree_reductions += 1;
    }
    let saved = ctx.origins.clone();
    ctx.origins = labels.iter().map(|&u| saved[u]).collect();
    let reduced = solve_core(&active, ctx, depth);
    ctx.origins = saved;
    let reduced = reduced?;
    let mut colors = vec![255; t.n()];
    for (i, &u) in labels.iter().enumerate() {
        colors[u] = reduced[i];
    }
    for (u, neighbors, merge) in undo.into_iter().rev() {
        ctx.tick()?;
        if let Some((a, b)) = merge {
            colors[b] = colors[a];
        }
        let mut mask = 15u8;
        for v in neighbors {
            if colors[v] >= 4 {
                return Err("Uncolored undo neighbor.".into());
            }
            mask &= !(1 << colors[v]);
        }
        if mask == 0 {
            return Err("Degree-four minor failed to restore.".into());
        }
        colors[u] = mask.trailing_zeros() as u8;
    }
    if colors.iter().any(|c| *c >= 4) || t.ends.iter().any(|p| colors[p[0]] == colors[p[1]]) {
        return Err("Minor reconstruction failed independent edge validation.".into());
    }
    ctx.publish(&colors);
    Ok(colors)
}

fn solve_core(t: &Plane, ctx: &mut Context, depth: usize) -> Result<Vec<u8>> {
    ctx.check()?;
    ctx.counts.max_depth = ctx.counts.max_depth.max(depth);
    t.validate(true)?;
    if t.n() <= 5 {
        let mut colors = vec![255; t.n()];
        colors[0] = 0;
        if extend_small(t, &mut colors, ctx)? {
            return Ok(colors);
        }
        return Err("A small plane triangulation was not colorable.".into());
    }
    // Parallel edges and low-degree vertices are the first cases of 6.5.
    for u in 0..t.n() {
        let mut seen = HashSet::new();
        for &d in &t.rot[u] {
            let v = t.target(d);
            if !seen.insert(v) {
                return solve_split(t, &[u, v], ctx, depth);
            }
        }
    }
    for u in 0..t.n() {
        if t.rot[u].len() <= 4 {
            let c: Vec<_> = t.rot[u].iter().map(|&d| t.target(d)).collect();
            // A chord of a neighbor circuit gives a smaller short circuit.
            if c.len() >= 3
                && (0..c.len()).all(|i| {
                    (i + 1..c.len()).all(|j| {
                        j == i + 1 || (i == 0 && j == c.len() - 1) || t.edge(c[i], c[j]).is_none()
                    })
                })
            {
                return solve_split(t, &c, ctx, depth);
            }
            let c = short_circuit(t, ctx)?.ok_or("Low-degree vertex without short circuit.")?;
            return solve_split(t, &c, ctx, depth);
        }
    }
    for k in catalogue() {
        for anchor in 0..t.n() {
            let degree = k.graph.rot[k.ring].len();
            if t.rot[anchor].len() != degree {
                continue;
            }
            for reverse in [false, true] {
                for offset in 0..degree {
                    let Some(map) = match_configuration(t, k, anchor, offset, reverse, ctx)? else {
                        continue;
                    };
                    let edges: Vec<_> = if k.contract.is_empty() {
                        vec![t
                            .edge(map[k.ring], map[k.graph.target(k.graph.rot[k.ring][0])])
                            .unwrap()]
                    } else {
                        k.contract
                            .iter()
                            .map(|&e| {
                                let [u, v] = k.graph.ends[e];
                                t.edge(map[u], map[v]).unwrap()
                            })
                            .collect()
                    };
                    match t.contract(&edges) {
                        Ok((child, vm)) => {
                            return solve_reduction(t, k, &map, child, vm, ctx, depth)
                        }
                        Err(_) => {
                            let cycle = short_circuit(t, ctx)?
                                .ok_or("Invalid contract without a short circuit (3.5).")?;
                            return solve_split(t, &cycle, ctx, depth);
                        }
                    }
                }
            }
        }
    }
    let cycle = short_circuit(t, ctx)?.ok_or("No RSST configuration or short circuit located.")?;
    solve_split(t, &cycle, ctx, depth)
}

pub struct RsstResult {
    pub status: &'static str,
    pub colors: Vec<u8>,
    pub solver_ms: f64,
    pub message: Option<String>,
    pub counters: Counters,
    pub stats: [f64; 13],
    pub threads: usize,
    pub winning_worker: Option<usize>,
    pub worker_ms: f64,
}
/// A connected simple graph with a spherical rotation system. Nontriangular
/// faces are capped with temporary vertices; returned colors omit those caps.
pub fn benchmark(rotation: &[Vec<usize>], budget_ms: f64, reporting: bool) -> Result<RsstResult> {
    benchmark_ordered(rotation, budget_ms, reporting, 0)
}
pub fn benchmark_ordered(
    rotation: &[Vec<usize>],
    budget_ms: f64,
    reporting: bool,
    variant: u32,
) -> Result<RsstResult> {
    benchmark_cancel(rotation, budget_ms, reporting, variant, None)
}
fn benchmark_cancel(
    rotation: &[Vec<usize>],
    budget_ms: f64,
    reporting: bool,
    variant: u32,
    cancel: Option<std::sync::Arc<std::sync::atomic::AtomicBool>>,
) -> Result<RsstResult> {
    if rotation.is_empty()
        || rotation.len() > 16384
        || !budget_ms.is_finite()
        || budget_ms <= 0.0
        || budget_ms > 100000.0
    {
        return Err("Invalid RSST size or time budget.".into());
    }
    let original = Plane::from_neighbors(rotation)?;
    let _ = catalogue(); // Fixed catalogue loading is preprocessing, not coloring.
    let mut run = super::Run::new(original.n(), budget_ms, reporting);
    run.variant = variant;
    run.cancel = cancel;
    run.phase = 2;
    let mut ctx = Context {
        run: &mut run,
        counts: Counters::default(),
        origins: (0..original.n()).map(Some).collect(),
        live_bytes: 0,
    };
    let mut graph = original.clone();
    let solved: Result<Vec<u8>> = (|| {
        if graph.n() <= 2 {
            return Ok((0..graph.n()).map(|u| u as u8).collect::<Vec<_>>());
        }
        // Caps also work for boundary walks with repeated vertices/edges.
        let faces = graph.faces()?;
        for face in faces {
            if face.len() != 3 {
                ctx.check()?;
                graph.cap(&face)?;
            }
        }
        ctx.origins.resize(graph.n(), None);
        ctx.counts.triangulation_vertices = graph.n();
        let mut colors = solve_t(&graph, &mut ctx, 0)?;
        colors.truncate(original.n());
        if original.ends.iter().any(|p| colors[p[0]] == colors[p[1]]) {
            return Err("Final independent edge check failed.".into());
        }
        ctx.check()?;
        Ok(colors)
    })();
    let elapsed = super::now() - ctx.run.started;
    let (status, colors, message) = match solved {
        Ok(colors) => ("complete", colors, None),
        Err(error) if error == "Timeout" => (
            "timeout",
            Vec::new(),
            Some("100-second or requested trial deadline reached.".into()),
        ),
        Err(error) if error == "Cancelled" => ("cancelled", Vec::new(), None),
        Err(error) if error.starts_with("Resource limit:") => {
            ("resource-limit", Vec::new(), Some(error))
        }
        Err(error) => ("error", Vec::new(), Some(error)),
    };
    Ok(RsstResult {
        status,
        colors,
        solver_ms: elapsed,
        message,
        stats: ctx.run.stats.values(elapsed),
        counters: ctx.counts,
        threads: 1,
        winning_worker: if status == "complete" { Some(1) } else { None },
        worker_ms: elapsed,
    })
}

/// Independent reduction-order portfolio, matching the browser worker model.
#[cfg(not(target_arch = "wasm32"))]
pub fn benchmark_threads(
    rotation: &[Vec<usize>],
    budget_ms: f64,
    reporting: bool,
    threads: usize,
) -> Result<RsstResult> {
    use std::sync::{
        atomic::{AtomicBool, Ordering},
        mpsc, Arc,
    };
    if !(1..=8).contains(&threads) {
        return Err("Use 1 to 8 threads.".into());
    }
    if threads == 1 {
        return benchmark(rotation, budget_ms, reporting);
    }
    if !budget_ms.is_finite() || budget_ms <= 0.0 || budget_ms > 100000.0 {
        return Err("Invalid time budget.".into());
    }
    let original = Plane::from_neighbors(rotation)?;
    let _ = catalogue();
    let cancel = Arc::new(AtomicBool::new(false));
    let started = super::now();
    let deadline = started + budget_ms;
    let (sender, receiver) = mpsc::channel();
    let mut winner = None;
    let mut last = None;
    let mut spawn_error = None;
    std::thread::scope(|scope| {
        for variant in 0..threads {
            let sender = sender.clone();
            let flag = Arc::clone(&cancel);
            let spawned = std::thread::Builder::new()
                .name(format!("rsst-{}", variant + 1))
                .stack_size(8 * 1024 * 1024)
                .spawn_scoped(scope, move || {
                    let remaining = (deadline - super::now()).max(0.000001);
                    let result = benchmark_cancel(
                        rotation,
                        remaining,
                        reporting && variant == 0,
                        variant as u32,
                        Some(flag),
                    );
                    let _ = sender.send((variant, result));
                });
            if let Err(e) = spawned {
                spawn_error = Some(e.to_string());
                cancel.store(true, Ordering::Relaxed);
                break;
            }
        }
        drop(sender);
        for (variant, result) in receiver {
            if winner.is_some() {
                continue;
            }
            match result {
                Ok(mut r) => {
                    if r.status == "complete"
                        && super::now() <= deadline
                        && r.colors.len() == original.n()
                        && r.colors.iter().all(|c| *c < 4)
                        && original
                            .ends
                            .iter()
                            .all(|p| r.colors[p[0]] != r.colors[p[1]])
                    {
                        r.winning_worker = Some(variant + 1);
                        winner = Some(r);
                        cancel.store(true, Ordering::Relaxed);
                    } else {
                        last = Some(r);
                    }
                }
                Err(e) => {
                    spawn_error = Some(e);
                    cancel.store(true, Ordering::Relaxed);
                }
            }
        }
    });
    if let Some(e) = spawn_error {
        return Err(e);
    }
    let mut r = winner.or(last).ok_or("No RSST worker result.")?;
    r.threads = threads;
    r.solver_ms = super::now() - started;
    if r.status != "complete" || r.solver_ms > budget_ms {
        r.colors.clear();
        r.winning_worker = None;
        if r.solver_ms > budget_ms {
            r.status = "timeout";
            r.message = Some("Shared trial deadline reached.".into());
        }
    }
    Ok(r)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn all_633_configurations_parse() {
        let c = catalogue();
        assert_eq!(c.len(), 633);
        assert!(c.iter().any(|k| k.contract.len() == 4));
        for k in c {
            k.graph.validate(false).unwrap();
        }
    }
    #[test]
    fn tetrahedron_rotations_and_contraction() {
        let g =
            Plane::from_neighbors(&[vec![1, 2, 3], vec![0, 3, 2], vec![0, 1, 3], vec![0, 2, 1]])
                .unwrap();
        g.validate(true).unwrap();
        let (h, map) = g.contract(&[g.edge(0, 1).unwrap()]).unwrap();
        assert_eq!(h.n(), 3);
        assert_eq!(map[0], map[1]);
        h.validate(true).unwrap();
        let colors = h.vertex_colors(&[1, 2, 3]).unwrap();
        assert!(colors.iter().all(|c| *c < 4));
    }
    #[test]
    fn every_catalogue_contract_restores() {
        let mut checked = 0;
        let mut c_checked = 0;
        for k in catalogue() {
            let mut t = k.graph.clone();
            let r: Vec<_> = (0..k.ring).collect();
            let face = t.outer_face(&ring_edges(&t, &r).unwrap()).unwrap();
            t.cap(&face).unwrap();
            t.validate(true).unwrap();
            let x = if k.contract.is_empty() {
                vec![k.graph.rot[k.ring][0] / 2]
            } else {
                k.contract.clone()
            };
            let Ok((child, vm)) = t.contract(&x) else {
                continue;
            };
            let mut run = super::super::Run::new(t.n(), 100000.0, false);
            let mut ctx = Context {
                run: &mut run,
                counts: Counters::default(),
                origins: (0..t.n()).map(Some).collect(),
                live_bytes: 0,
            };
            assert!(
                match_configuration(&t, k, k.ring, 0, false, &mut ctx)
                    .unwrap()
                    .is_some(),
                "{}",
                k.name
            );
            let mut mirror = t.clone();
            for row in &mut mirror.rot {
                row.reverse();
            }
            assert!(
                match_configuration(&mirror, k, k.ring, t.rot[k.ring].len() - 1, true, &mut ctx)
                    .unwrap()
                    .is_some(),
                "mirror {}",
                k.name
            );
            let colors = solve_reduction(
                &t,
                k,
                &(0..k.graph.n()).collect::<Vec<_>>(),
                child,
                vm,
                &mut ctx,
                0,
            )
            .unwrap_or_else(|e| panic!("{}: {e}", k.name));
            assert!(colors.iter().all(|c| *c < 4));
            assert!(t.ends.iter().all(|p| colors[p[0]] != colors[p[1]]));
            checked += 1;
            if !k.contract.is_empty() {
                c_checked += 1;
            }
        }
        assert_eq!(checked, 633);
        assert!(c_checked > 0);
    }
    /// Small bipyramids have a prescribed separating k-cycle. Subdivide a
    /// face on each side of the five-cycle so it satisfies the shortness rule.
    fn bipyramid(k: usize) -> Plane {
        let mut rows = vec![Vec::new(); k + 2];
        rows[k] = (0..k).collect();
        rows[k + 1] = (0..k).rev().collect();
        for i in 0..k {
            rows[i] = vec![(i + 1) % k, k, (i + k - 1) % k, k + 1];
        }
        Plane::from_neighbors(&rows).unwrap()
    }
    #[test]
    fn explicit_short_circuits_3_4_5() {
        for k in 3..=5 {
            let mut t = bipyramid(k);
            if k == 5 {
                for apex in [k, k + 1] {
                    let face = t
                        .faces()
                        .unwrap()
                        .into_iter()
                        .find(|f| f.iter().any(|&d| t.source(d) == apex))
                        .unwrap();
                    t.cap(&face).unwrap();
                }
            }
            t.validate(true).unwrap();
            assert!(is_short(&t, &(0..k).collect::<Vec<_>>()));
            let mut run = super::super::Run::new(t.n(), 1000.0, false);
            let mut ctx = Context {
                run: &mut run,
                counts: Counters::default(),
                origins: (0..t.n()).map(Some).collect(),
                live_bytes: 0,
            };
            let colors = solve_split(&t, &(0..k).collect::<Vec<_>>(), &mut ctx, 0).unwrap();
            assert!(t.ends.iter().all(|p| colors[p[0]] != colors[p[1]]));
            assert!(ctx.counts.separators[k - 2] > 0);
        }
    }
    #[test]
    fn mirrors_and_invalid_surface() {
        let g = bipyramid(5);
        let rows: Vec<Vec<usize>> = g
            .rot
            .iter()
            .map(|r| r.iter().map(|&d| g.target(d)).collect())
            .collect();
        for rows in [
            rows.clone(),
            rows.iter()
                .map(|r| r.iter().copied().rev().collect())
                .collect(),
        ] {
            let result = benchmark(&rows, 1000.0, false).unwrap();
            assert_eq!(result.status, "complete", "{:?}", result.message);
        }
        let mut bad = rows;
        bad[0].swap(0, 1);
        assert!(benchmark(&bad, 1000.0, false).is_err());
    }
    #[test]
    fn all_five_ring_augmentations() {
        let t = bipyramid(5);
        let mut keep = vec![true; t.n()];
        keep[6] = false;
        let h = t.induced(&keep).0;
        let ring = ring_edges(&h, &[0, 1, 2, 3, 4]).unwrap();
        for i in 0..5 {
            for kind in 0..3 {
                let (g, vm) = augment_five(&h, &[0, 1, 2, 3, 4], i, kind).unwrap();
                g.validate(true).unwrap();
                let mut run = super::super::Run::new(g.n(), 1000.0, false);
                let mut ctx = Context {
                    run: &mut run,
                    counts: Counters::default(),
                    origins: (0..g.n()).map(Some).collect(),
                    live_bytes: 0,
                };
                let colors = solve_t(&g, &mut ctx, 0).unwrap();
                let mut edge = lift_child(&h, &g, &colors, vm.as_deref());
                let key = canonical(&mut edge, &ring);
                let pairs = match kind {
                    0 => vec![
                        (i, (i + 4) % 5),
                        (i, (i + 1) % 5),
                        ((i + 4) % 5, (i + 1) % 5),
                    ],
                    1 => vec![
                        ((i + 1) % 5, (i + 3) % 5),
                        ((i + 1) % 5, (i + 4) % 5),
                        ((i + 2) % 5, (i + 3) % 5),
                        ((i + 2) % 5, (i + 4) % 5),
                    ],
                    _ => (0..5).map(|a| (a, (a + 1) % 5)).collect(),
                };
                assert!(
                    pairs.iter().any(|&(a, b)| pair_pattern(a, b) == key),
                    "i={i}, family={kind}"
                );
            }
        }
    }
    #[test]
    fn projection_with_identified_ring_vertices() {
        let k = &catalogue()[0];
        let mut t = k.graph.clone();
        let face = t
            .outer_face(&ring_edges(&t, &(0..k.ring).collect::<Vec<_>>()).unwrap())
            .unwrap();
        let chord = t.add_chord(&face, 0, 3).unwrap();
        for face in t.faces().unwrap() {
            if face.len() != 3 {
                t.cap(&face).unwrap();
            }
        }
        let (t, projection) = t.contract(&[chord]).unwrap();
        assert_eq!(projection[0], projection[3]);
        let mut run = super::super::Run::new(t.n(), 10000.0, false);
        let mut ctx = Context {
            run: &mut run,
            counts: Counters::default(),
            origins: (0..t.n()).map(Some).collect(),
            live_bytes: 0,
        };
        let root = projection[k.ring];
        let mapped = (0..t.rot[root].len())
            .find_map(|offset| match_configuration(&t, k, root, offset, false, &mut ctx).unwrap())
            .unwrap();
        assert!(mapped[..k.ring].iter().collect::<HashSet<_>>().len() < k.ring);
        let e = t
            .edge(
                mapped[k.ring],
                mapped[k.graph.target(k.graph.rot[k.ring][0])],
            )
            .unwrap();
        let (child, vm) = t.contract(&[e]).unwrap();
        let colors = solve_reduction(&t, k, &mapped, child, vm, &mut ctx, 0).unwrap();
        assert!(t.ends.iter().all(|p| colors[p[0]] != colors[p[1]]));
    }
}
