//! The two Fourfold benchmark solvers. This is a port of the demo methods,
//! not a complete implementation of either historical four-color algorithm.
use std::cell::RefCell;

#[cfg(target_arch = "wasm32")]
#[link(wasm_import_module = "host")]
extern "C" {
    fn time_now() -> f64;
    fn progress(colors: *const i8, len: u32, elapsed: f64, phase: u32, stats: *const f64);
}

fn now() -> f64 {
    #[cfg(target_arch = "wasm32")]
    // The embedding worker supplies a monotonic performance.now clock.
    unsafe {
        time_now()
    }
    #[cfg(not(target_arch = "wasm32"))]
    {
        use std::sync::OnceLock;
        use std::time::Instant;
        static START: OnceLock<Instant> = OnceLock::new();
        START.get_or_init(Instant::now).elapsed().as_secs_f64() * 1000.0
    }
}

#[derive(Default, Clone)]
struct Stats {
    probes: u64,
    decisions: u64,
    backtracks: u64,
    batches: u64,
    removed: usize,
    swaps: u64,
    kempe_visits: u64,
    core_vertices: usize,
    reduction_ms: f64,
    core_ms: f64,
    restoration_ms: f64,
    progress_updates: u64,
}
impl Stats {
    // Stable ABI order; the final field is elapsed solver time.
    fn values(&self, elapsed: f64) -> [f64; 13] {
        [
            self.probes as f64,
            self.decisions as f64,
            self.backtracks as f64,
            self.batches as f64,
            self.removed as f64,
            self.swaps as f64,
            self.kempe_visits as f64,
            self.core_vertices as f64,
            self.reduction_ms,
            self.core_ms,
            self.restoration_ms,
            self.progress_updates as f64,
            elapsed,
        ]
    }
}
#[derive(Debug, Clone, Copy, PartialEq)]
enum Failure {
    Timeout = 1,
    Unsupported = 2,
    Invalid = 3,
}
type Outcome = Result<(), Failure>;

struct Run {
    colors: Vec<i8>,
    stats: Stats,
    started: f64,
    deadline: f64,
    next_update: f64,
    phase: u32,
    snapshot_safe: bool,
    reporting: bool,
}
impl Run {
    fn new(n: usize, budget: f64, reporting: bool) -> Self {
        let started = now();
        Self {
            colors: vec![-1; n],
            stats: Stats::default(),
            started,
            deadline: started + budget,
            next_update: started + 5000.0,
            phase: 0,
            snapshot_safe: true,
            reporting,
        }
    }
    fn report(&mut self, stamp: f64, force: bool) {
        if !self.reporting || !self.snapshot_safe || (!force && stamp < self.next_update) {
            return;
        }
        self.next_update = stamp + 5000.0;
        self.stats.progress_updates += 1;
        #[cfg(not(target_arch = "wasm32"))]
        eprintln!(
            "{:.1}s: {}/{} colored, {} search choices, {} backtracks",
            (stamp - self.started) / 1000.0,
            self.colors.iter().filter(|&&c| c >= 0).count(),
            self.colors.len(),
            self.stats.decisions,
            self.stats.backtracks
        );
        #[cfg(target_arch = "wasm32")]
        {
            let stats = self.stats.values(stamp - self.started);
            // The host copies both borrowed arrays synchronously. It must not
            // retain the pointers or call back into the module while solving.
            unsafe {
                progress(
                    self.colors.as_ptr(),
                    self.colors.len() as u32,
                    stamp - self.started,
                    self.phase,
                    stats.as_ptr(),
                );
            }
        }
    }
    fn check(&mut self) -> Outcome {
        let stamp = now();
        if stamp > self.deadline {
            return Err(Failure::Timeout);
        }
        self.report(stamp, false);
        Ok(())
    }
    #[inline]
    fn tick(&mut self) -> Outcome {
        self.stats.probes += 1;
        if self.stats.probes & 4095 == 0 {
            self.check()?;
        }
        Ok(())
    }
}

struct Frame {
    vertex: usize,
    left: u8,
    assigned: Option<usize>,
}
fn assign(
    adj: &[Vec<usize>],
    run: &mut Run,
    counts: &mut [u32],
    mask: &mut [u8],
    u: usize,
    c: usize,
    add: bool,
) -> Outcome {
    run.colors[u] = if add { c as i8 } else { -1 };
    for &v in &adj[u] {
        let index = v * 4 + c;
        if add {
            if counts[index] == 0 {
                mask[v] |= 1 << c;
            }
            counts[index] += 1;
        } else {
            counts[index] -= 1;
            if counts[index] == 0 {
                mask[v] &= !(1 << c);
            }
        }
        run.tick()?;
    }
    Ok(())
}
fn dsatur(adj: &[Vec<usize>], vertices: &[usize], run: &mut Run) -> Outcome {
    let mut counts = vec![0u32; adj.len() * 4];
    let mut mask = vec![0u8; adj.len()];
    let mut stack: Vec<Frame> = Vec::new();
    let mut forward = true;
    loop {
        if forward {
            let mut best = None;
            let (mut saturation, mut degree) = (0, 0);
            for &u in vertices {
                run.tick()?;
                if run.colors[u] < 0 {
                    let (s, d) = (mask[u].count_ones(), adj[u].len());
                    if best.is_none() || s > saturation || (s == saturation && d > degree) {
                        best = Some(u);
                        saturation = s;
                        degree = d;
                    }
                }
            }
            let Some(u) = best else {
                return Ok(());
            };
            stack.push(Frame {
                vertex: u,
                left: 15 & !mask[u],
                assigned: None,
            });
        }
        forward = false;
        while let Some(frame) = stack.last_mut() {
            if let Some(c) = frame.assigned.take() {
                assign(adj, run, &mut counts, &mut mask, frame.vertex, c, false)?;
            }
            if frame.left != 0 {
                let c = frame.left.trailing_zeros() as usize;
                frame.left &= !(1 << c);
                frame.assigned = Some(c);
                assign(adj, run, &mut counts, &mut mask, frame.vertex, c, true)?;
                run.stats.decisions += 1;
                forward = true;
                break;
            }
            stack.pop();
            run.stats.backtracks += 1;
        }
        if !forward {
            return Err(Failure::Unsupported);
        }
    }
}
fn classic(adj: &[Vec<usize>], run: &mut Run) -> Outcome {
    run.phase = 1;
    run.stats.core_vertices = adj.len();
    let vertices: Vec<usize> = (0..adj.len()).collect();
    let started = now();
    dsatur(adj, &vertices, run)?;
    run.stats.core_ms = now() - started;
    Ok(())
}
fn enqueue(
    u: usize,
    active: &[bool],
    degree: &[usize],
    queued: &mut [usize],
    round: usize,
    next: &mut Vec<usize>,
) {
    if active[u] && degree[u] <= 4 && queued[u] != round {
        queued[u] = round;
        next.push(u);
    }
}
fn reduction(adj: &[Vec<usize>], run: &mut Run) -> Outcome {
    let n = adj.len();
    run.phase = 2;
    let mut active = vec![true; n];
    let mut degree: Vec<usize> = adj.iter().map(Vec::len).collect();
    let (mut blocked, mut queued) = (vec![0; n], vec![0; n]);
    let mut removed = Vec::with_capacity(n);
    let mut eligible: Vec<usize> = (0..n).filter(|&u| degree[u] <= 4).collect();
    let mut round = 0;
    let mut started = now();
    while !eligible.is_empty() {
        round += 1;
        let mut batch = Vec::new();
        for &u in &eligible {
            run.tick()?;
            if active[u] && degree[u] <= 4 && blocked[u] != round {
                batch.push(u);
                blocked[u] = round;
                for &v in &adj[u] {
                    if active[v] {
                        blocked[v] = round;
                    }
                }
            }
        }
        if batch.is_empty() {
            return Err(Failure::Invalid);
        }
        for &u in &batch {
            active[u] = false;
            removed.push(u);
        }
        run.stats.removed = removed.len();
        let mut next = Vec::new();
        for &u in &batch {
            for &v in &adj[u] {
                run.tick()?;
                if active[v] {
                    degree[v] -= 1;
                    enqueue(v, &active, &degree, &mut queued, round, &mut next);
                }
            }
        }
        for &u in &eligible {
            enqueue(u, &active, &degree, &mut queued, round, &mut next);
        }
        eligible = next;
        run.stats.batches += 1;
    }
    run.stats.reduction_ms = now() - started;
    let core: Vec<usize> = (0..n).filter(|&u| active[u]).collect();
    run.stats.core_vertices = core.len();
    run.phase = 3;
    started = now();
    if !core.is_empty() {
        dsatur(adj, &core, run)?;
    }
    run.stats.core_ms = now() - started;
    run.phase = 4;
    started = now();
    let (mut seen, mut queue) = (vec![0u64; n], vec![0usize; n]);
    let mut stamp = 0;
    for &u in removed.iter().rev() {
        let mut ring = Vec::with_capacity(4);
        let mut used = 0u8;
        for &v in &adj[u] {
            if run.colors[v] >= 0 {
                ring.push(v);
                used |= 1 << run.colors[v];
            }
        }
        if ring.len() > 4 {
            return Err(Failure::Invalid);
        }
        let mut free = 15 & !used;
        if free == 0 {
            'pairs: for a in 0..ring.len() {
                for b in a + 1..ring.len() {
                    let (start, target) = (ring[a], ring[b]);
                    let (ca, cb) = (run.colors[start], run.colors[target]);
                    stamp += 1;
                    let (mut head, mut tail) = (0, 1);
                    queue[0] = start;
                    seen[start] = stamp;
                    while head < tail {
                        let x = queue[head];
                        head += 1;
                        run.stats.kempe_visits += 1;
                        for &y in &adj[x] {
                            run.tick()?;
                            if seen[y] != stamp && (run.colors[y] == ca || run.colors[y] == cb) {
                                seen[y] = stamp;
                                queue[tail] = y;
                                tail += 1;
                            }
                        }
                    }
                    if seen[target] != stamp {
                        run.snapshot_safe = false;
                        for &v in &queue[..tail] {
                            run.colors[v] = if run.colors[v] == ca { cb } else { ca };
                            run.tick()?;
                        }
                        run.snapshot_safe = true;
                        free = 1 << ca;
                        run.stats.swaps += 1;
                        break 'pairs;
                    }
                }
            }
            if free == 0 {
                return Err(Failure::Unsupported);
            }
        }
        run.colors[u] = free.trailing_zeros() as i8;
        run.tick()?;
    }
    run.stats.restoration_ms = now() - started;
    Ok(())
}

fn solve_graph(adj: &[Vec<usize>], method: u32, budget: f64, reporting: bool) -> (u32, Run, f64) {
    let mut run = Run::new(adj.len(), budget, reporting);
    let outcome = match method {
        0 => classic(adj, &mut run),
        1 => reduction(adj, &mut run),
        _ => Err(Failure::Invalid),
    }
    .and_then(|()| run.check());
    let status = match outcome {
        Ok(()) => 0,
        Err(error) => {
            run.report(now(), true);
            error as u32
        }
    };
    let elapsed = now() - run.started;
    (status, run, elapsed)
}
fn graph_from_edges(n: usize, edges: &[u32]) -> Result<Vec<Vec<usize>>, Failure> {
    if n == 0 || n > 16384 || edges.len() % 2 != 0 || edges.len() > n * 16 {
        return Err(Failure::Invalid);
    }
    let mut adj = vec![Vec::new(); n];
    for pair in edges.chunks_exact(2) {
        let (u, v) = (pair[0] as usize, pair[1] as usize);
        if u >= n || v >= n || u == v {
            return Err(Failure::Invalid);
        }
        adj[u].push(v);
        adj[v].push(u);
    }
    for neighbors in &mut adj {
        neighbors.sort_unstable();
        if neighbors.windows(2).any(|p| p[0] == p[1]) {
            return Err(Failure::Invalid);
        }
    }
    Ok(adj)
}

#[derive(Default)]
struct Store {
    n: usize,
    input: Vec<u32>,
    graph: Option<Vec<Vec<usize>>>,
    colors: Vec<i8>,
    stats: [f64; 13],
}

/// Native CLI API using exactly the same solver core as the WASM exports.
pub struct Benchmark {
    pub status: &'static str,
    pub colors: Vec<i8>,
    pub solver_ms: f64,
    pub decisions: u64,
    pub backtracks: u64,
    pub removed: usize,
    pub swaps: u64,
}
pub fn benchmark(
    n: usize,
    edges: &[u32],
    method: &str,
    budget_ms: f64,
    reporting: bool,
) -> Result<Benchmark, String> {
    let method_id = match method {
        "dsatur" => 0,
        "reduction" => 1,
        _ => return Err("Unknown method; choose dsatur or reduction.".into()),
    };
    if !budget_ms.is_finite() || budget_ms <= 0.0 || budget_ms > 100000.0 {
        return Err("Timeout must be between 0 and 100 seconds.".into());
    }
    let graph = graph_from_edges(n, edges)
        .map_err(|_| "Invalid graph: check sizes, endpoint IDs, loops, and duplicate edges.")?;
    let (status, run, elapsed) = solve_graph(&graph, method_id, budget_ms, reporting);
    if status == 0 {
        if !run.colors.iter().all(|&c| (0..4).contains(&c))
            || edges
                .chunks_exact(2)
                .any(|p| run.colors[p[0] as usize] == run.colors[p[1] as usize])
        {
            return Err("Final coloring failed independent edge validation.".into());
        }
    }
    Ok(Benchmark {
        status: match status {
            0 => "complete",
            1 => "timeout",
            2 => "unsupported",
            _ => "error",
        },
        colors: if status == 0 { run.colors } else { Vec::new() },
        solver_ms: elapsed,
        decisions: run.stats.decisions,
        backtracks: run.stats.backtracks,
        removed: run.stats.removed,
        swaps: run.stats.swaps,
    })
}
thread_local! { static STORE: RefCell<Store> = RefCell::new(Store::default()); }

/// Allocate a bounded input buffer. JavaScript fills it with undirected edge pairs.
#[no_mangle]
pub extern "C" fn allocate_graph(n: u32, edge_values: u32) -> *mut u32 {
    STORE.with(|cell| {
        let mut store = cell.borrow_mut();
        *store = Store::default();
        if n == 0 || n > 16384 || edge_values % 2 != 0 || edge_values > n * 16 {
            return std::ptr::null_mut();
        }
        store.n = n as usize;
        store.input = vec![0; edge_values as usize];
        store.input.as_mut_ptr()
    })
}
#[no_mangle]
pub extern "C" fn prepare_graph() -> u32 {
    STORE.with(|cell| {
        let mut store = cell.borrow_mut();
        match graph_from_edges(store.n, &store.input) {
            Ok(graph) => {
                store.graph = Some(graph);
                0
            }
            Err(error) => {
                store.graph = None;
                error as u32
            }
        }
    })
}
#[no_mangle]
pub extern "C" fn solve(method: u32, budget_ms: f64, reporting: u32) -> u32 {
    STORE.with(|cell| {
        let mut store = cell.borrow_mut();
        store.colors.clear();
        store.stats = [0.0; 13];
        let Some(graph) = store.graph.as_ref() else {
            return Failure::Invalid as u32;
        };
        if method > 1 || !budget_ms.is_finite() || budget_ms <= 0.0 {
            return Failure::Invalid as u32;
        }
        let (status, run, elapsed) =
            solve_graph(graph, method, budget_ms.min(100000.0), reporting != 0);
        store.stats = run.stats.values(elapsed);
        if status == 0 {
            store.colors = run.colors;
        }
        status
    })
}
#[no_mangle]
pub extern "C" fn colors_ptr() -> *const i8 {
    STORE.with(|cell| cell.borrow().colors.as_ptr())
}
#[no_mangle]
pub extern "C" fn colors_len() -> u32 {
    STORE.with(|cell| cell.borrow().colors.len() as u32)
}
#[no_mangle]
pub extern "C" fn stats_ptr() -> *const f64 {
    STORE.with(|cell| cell.borrow().stats.as_ptr())
}
#[no_mangle]
pub extern "C" fn abi_version() -> u32 {
    1
}

#[cfg(test)]
mod tests {
    use super::*;
    fn check(n: usize, edges: &[u32]) {
        let graph = graph_from_edges(n, edges).unwrap();
        for method in 0..=1 {
            let (status, run, _) = solve_graph(&graph, method, 1000.0, false);
            assert_eq!(status, 0);
            assert!(run.colors.iter().all(|&c| (0..4).contains(&c)));
            for pair in edges.chunks_exact(2) {
                assert_ne!(run.colors[pair[0] as usize], run.colors[pair[1] as usize]);
            }
        }
    }
    #[test]
    fn fixtures() {
        check(3, &[0, 1, 1, 2, 2, 0]);
        check(4, &[0, 1, 0, 2, 0, 3, 1, 2, 1, 3, 2, 3]);
        check(
            6,
            &[
                2, 3, 3, 4, 4, 5, 5, 2, 0, 2, 0, 3, 0, 4, 0, 5, 1, 2, 1, 3, 1, 4, 1, 5,
            ],
        );
        check(1, &[]);
    }
    #[test]
    fn invalid_graphs() {
        assert!(graph_from_edges(0, &[]).is_err());
        assert!(graph_from_edges(2, &[0, 2]).is_err());
        assert!(graph_from_edges(2, &[0, 0]).is_err());
        assert!(graph_from_edges(2, &[0, 1, 1, 0]).is_err());
    }
    #[test]
    fn unsupported_k5() {
        let mut edges = Vec::new();
        for u in 0..5 {
            for v in u + 1..5 {
                edges.extend([u, v]);
            }
        }
        let graph = graph_from_edges(5, &edges).unwrap();
        for method in 0..=1 {
            assert_eq!(solve_graph(&graph, method, 1000.0, false).0, 2);
        }
    }
}
