//! Rotation-system operations. Dart 2e runs from ends[e][0] to ends[e][1].
use std::collections::{HashMap, VecDeque};

#[derive(Clone, Debug)]
pub struct Plane {
    pub ends: Vec<[usize; 2]>,
    pub rot: Vec<Vec<usize>>,
}

impl Plane {
    pub fn from_neighbors(neighbors: &[Vec<usize>]) -> Result<Self, String> {
        let n = neighbors.len();
        let max_darts = if n >= 3 {
            6 * n - 12
        } else {
            n.saturating_mul(n.saturating_sub(1))
        };
        if n == 0 || n > 16384 || neighbors.iter().map(Vec::len).sum::<usize>() > max_darts {
            return Err("Rotation exceeds the simple planar input size bounds.".into());
        }
        let mut ends = Vec::new();
        let mut darts = HashMap::new();
        for (u, row) in neighbors.iter().enumerate() {
            for &v in row {
                if v >= neighbors.len() || u == v || darts.contains_key(&(u, v)) {
                    return Err("Invalid rotation: endpoint, loop, or duplicate neighbor.".into());
                }
                let d = if u < v {
                    let d = ends.len() * 2;
                    ends.push([u, v]);
                    d
                } else {
                    *darts.get(&(v, u)).ok_or("Asymmetric rotation.")? ^ 1
                };
                darts.insert((u, v), d);
            }
        }
        let rot = neighbors
            .iter()
            .enumerate()
            .map(|(u, row)| row.iter().map(|&v| darts[&(u, v)]).collect())
            .collect();
        let g = Self { ends, rot };
        g.validate(false)?;
        Ok(g)
    }
    pub fn n(&self) -> usize {
        self.rot.len()
    }
    pub fn source(&self, d: usize) -> usize {
        self.ends[d / 2][d % 2]
    }
    pub fn target(&self, d: usize) -> usize {
        self.source(d ^ 1)
    }
    pub fn dart(&self, u: usize, v: usize) -> Option<usize> {
        self.rot[u].iter().copied().find(|&d| self.target(d) == v)
    }
    pub fn edge(&self, u: usize, v: usize) -> Option<usize> {
        self.dart(u, v).map(|d| d / 2)
    }
    pub fn faces(&self) -> Result<Vec<Vec<usize>>, String> {
        let mut next = vec![usize::MAX; self.ends.len() * 2];
        for (u, row) in self.rot.iter().enumerate() {
            for (i, &d) in row.iter().enumerate() {
                if d >= next.len() || self.source(d) != u || next[d ^ 1] != usize::MAX {
                    return Err("Invalid dart incidence.".into());
                }
                next[d ^ 1] = row[(i + row.len() - 1) % row.len()];
            }
        }
        if next.contains(&usize::MAX) {
            return Err("Missing twin dart.".into());
        }
        let mut used = vec![false; next.len()];
        let mut faces = Vec::new();
        for start in 0..next.len() {
            if used[start] {
                continue;
            }
            let mut face = Vec::new();
            let mut d = start;
            loop {
                if used[d] {
                    if d != start {
                        return Err("Invalid face walk.".into());
                    }
                    break;
                }
                used[d] = true;
                face.push(d);
                d = next[d];
            }
            faces.push(face);
        }
        Ok(faces)
    }
    pub fn validate(&self, triangles: bool) -> Result<(), String> {
        if self.n() == 0 {
            return Err("Empty plane graph.".into());
        }
        if self.n() == 1 && self.ends.is_empty() {
            return Ok(());
        }
        let faces = self.faces()?;
        let mut seen = vec![false; self.n()];
        let mut todo = vec![0];
        seen[0] = true;
        while let Some(u) = todo.pop() {
            for &d in &self.rot[u] {
                let v = self.target(d);
                if !seen[v] {
                    seen[v] = true;
                    todo.push(v);
                }
            }
        }
        if seen.contains(&false) {
            return Err("RSST needs a connected plane embedding.".into());
        }
        if self.n() + faces.len() != self.ends.len() + 2 {
            return Err("Rotation system is not an embedding on the sphere (Euler check).".into());
        }
        if triangles && faces.iter().any(|f| f.len() != 3) {
            return Err("Expected a triangulation.".into());
        }
        Ok(())
    }
    /// Keep specified vertices and edges; preserve their cyclic incidence order.
    /// Returns new graph and maps from old vertex/edge IDs to new IDs.
    pub fn restrict(&self, keep_v: &[bool], keep_e: &[bool]) -> (Self, Vec<usize>, Vec<usize>) {
        let mut vm = vec![usize::MAX; self.n()];
        let mut rot = Vec::new();
        for u in 0..self.n() {
            if keep_v[u] {
                vm[u] = rot.len();
                rot.push(Vec::new());
            }
        }
        let mut em = vec![usize::MAX; self.ends.len()];
        let mut ends = Vec::new();
        for (e, &[u, v]) in self.ends.iter().enumerate() {
            if keep_e[e] && keep_v[u] && keep_v[v] {
                em[e] = ends.len();
                ends.push([vm[u], vm[v]]);
            }
        }
        for u in 0..self.n() {
            if !keep_v[u] {
                continue;
            }
            for &d in &self.rot[u] {
                if em[d / 2] != usize::MAX {
                    rot[vm[u]].push(em[d / 2] * 2 + d % 2);
                }
            }
        }
        (Self { ends, rot }, vm, em)
    }
    pub fn induced(&self, keep: &[bool]) -> (Self, Vec<usize>, Vec<usize>) {
        self.restrict(keep, &vec![true; self.ends.len()])
    }
    pub fn components_without(&self, cycle: &[usize]) -> Vec<Vec<usize>> {
        let mut seen = vec![false; self.n()];
        for &u in cycle {
            seen[u] = true;
        }
        let mut out = Vec::new();
        for root in 0..self.n() {
            if seen[root] {
                continue;
            }
            seen[root] = true;
            let mut part = vec![root];
            let mut p = 0;
            while p < part.len() {
                let u = part[p];
                p += 1;
                for &d in &self.rot[u] {
                    let v = self.target(d);
                    if !seen[v] {
                        seen[v] = true;
                        part.push(v);
                    }
                }
            }
            out.push(part);
        }
        out
    }
    pub fn outer_face(&self, ring: &[usize]) -> Result<Vec<usize>, String> {
        for f in self.faces()? {
            if f.len() == ring.len()
                && (0..ring.len()).any(|start| {
                    (0..ring.len()).all(|i| ring[i] == f[(start + i) % ring.len()] / 2)
                        || (0..ring.len())
                            .all(|i| ring[i] == f[(start + ring.len() - i) % ring.len()] / 2)
                })
            {
                return Ok(f);
            }
        }
        Err("Ring does not trace a face.".into())
    }
    pub fn add_chord(&mut self, face: &[usize], u: usize, v: usize) -> Result<usize, String> {
        let i = face
            .iter()
            .position(|&d| self.source(d) == u)
            .ok_or("Chord endpoint not on face.")?;
        let j = face
            .iter()
            .position(|&d| self.source(d) == v)
            .ok_or("Chord endpoint not on face.")?;
        if i == j {
            return Err("Loop chord.".into());
        }
        let e = self.ends.len();
        self.ends.push([u, v]);
        for (index, w, d) in [(i, u, e * 2), (j, v, e * 2 + 1)] {
            let before = face[(index + face.len() - 1) % face.len()] ^ 1;
            let pos = self.rot[w]
                .iter()
                .position(|&x| x == before)
                .ok_or("Missing corner.")?;
            self.rot[w].insert(pos, d);
        }
        Ok(e)
    }
    pub fn cap(&mut self, face: &[usize]) -> Result<usize, String> {
        let apex = self.n();
        self.rot.push(Vec::new());
        for (i, &outgoing) in face.iter().enumerate() {
            let u = self.source(outgoing);
            let before = face[(i + face.len() - 1) % face.len()] ^ 1;
            let pos = self.rot[u]
                .iter()
                .position(|&x| x == before)
                .ok_or("Missing cap corner.")?;
            let e = self.ends.len();
            self.ends.push([u, apex]);
            self.rot[u].insert(pos, e * 2);
            self.rot[apex].push(e * 2 + 1);
        }
        Ok(apex)
    }
    /// Contract edges, then remove only facial digons. Separating parallel
    /// edges remain for the length-two circuit branch of RSST section 6.5.
    pub fn contract(&self, contracted: &[usize]) -> Result<(Self, Vec<usize>), String> {
        let mut g = self.clone();
        let mut vm: Vec<usize> = (0..self.n()).collect();
        let mut removed = vec![false; self.ends.len()];
        for &e in contracted {
            if e >= g.ends.len() || removed[e] {
                return Err("Invalid contract edge.".into());
            }
            let [u, v] = g.ends[e];
            if u == v {
                return Err("Contract contains a cycle.".into());
            }
            let i = g.rot[u]
                .iter()
                .position(|&d| d == 2 * e)
                .ok_or("Contract dart missing.")?;
            let j = g.rot[v]
                .iter()
                .position(|&d| d == 2 * e + 1)
                .ok_or("Contract twin missing.")?;
            let ru = &g.rot[u];
            let rv = &g.rot[v];
            let merged = (1..ru.len())
                .map(|k| ru[(i + k) % ru.len()])
                .chain((1..rv.len()).map(|k| rv[(j + k) % rv.len()]))
                .collect();
            g.rot[u] = merged;
            g.rot[v].clear();
            removed[e] = true;
            for pair in &mut g.ends {
                for x in pair {
                    if *x == v {
                        *x = u;
                    }
                }
            }
            for x in &mut vm {
                if *x == v {
                    *x = u;
                }
            }
        }
        if g.ends
            .iter()
            .enumerate()
            .any(|(e, p)| !removed[e] && p[0] == p[1])
        {
            return Err("Contract collapses a non-contract edge.".into());
        }
        let keep_v: Vec<bool> = g.rot.iter().map(|r| !r.is_empty()).collect();
        let keep_e: Vec<bool> = removed.iter().map(|b| !b).collect();
        let (mut h, compact, _) = g.restrict(&keep_v, &keep_e);
        for x in &mut vm {
            *x = compact[*x];
        }
        loop {
            let faces = h.faces()?;
            let mut remove = vec![false; h.ends.len()];
            for f in &faces {
                if f.len() == 2 {
                    remove[f[0] / 2] = true;
                }
            }
            if !remove.contains(&true) {
                break;
            }
            // One at a time: adjacent digons can share edges.
            let e = remove.iter().position(|b| *b).unwrap();
            let mut keep = vec![true; h.ends.len()];
            keep[e] = false;
            h = h.restrict(&vec![true; h.n()], &keep).0;
        }
        h.validate(true)?;
        Ok((h, vm))
    }
    pub fn edge_colors(&self, vertex: &[u8]) -> Vec<u8> {
        self.ends
            .iter()
            .map(|p| vertex[p[0]] ^ vertex[p[1]])
            .collect()
    }
    pub fn vertex_colors(&self, edge: &[u8]) -> Result<Vec<u8>, String> {
        if edge.len() != self.ends.len() || edge.iter().any(|c| !(1..=3).contains(c)) {
            return Err("Invalid tri-color range.".into());
        }
        let mut colors = vec![255; self.n()];
        for root in 0..self.n() {
            if colors[root] != 255 {
                continue;
            }
            colors[root] = 0;
            let mut queue = VecDeque::from([root]);
            while let Some(u) = queue.pop_front() {
                for &d in &self.rot[u] {
                    let v = self.target(d);
                    let c = colors[u] ^ edge[d / 2];
                    if colors[v] == 255 {
                        colors[v] = c;
                        queue.push_back(v);
                    } else if colors[v] != c {
                        return Err("Tri-coloring does not integrate to vertex colors.".into());
                    }
                }
            }
        }
        Ok(colors)
    }
}
