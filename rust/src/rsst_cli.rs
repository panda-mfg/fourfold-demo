use fourfold_engine::rsst;
use std::{env, fs, process};
fn main() {
    match run() {
        Ok(true) => (),
        Ok(false) => process::exit(2),
        Err(e) => {
            eprintln!("{e}");
            process::exit(1)
        }
    }
}
fn run() -> Result<bool, String> {
    let mut input = None;
    let mut timeout = 100.0;
    let mut json = false;
    let mut threads = 1;
    let mut args = env::args().skip(1);
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--input" => input = Some(args.next().ok_or("Missing input path")?),
            "--timeout" => {
                timeout = args
                    .next()
                    .ok_or("Missing seconds")?
                    .parse::<f64>()
                    .map_err(|_| "Invalid seconds")?
            }
            "--json" => json = true,
            "--threads" => {
                let t = args.next().ok_or("Missing thread count")?;
                threads = if t == "auto" {
                    std::thread::available_parallelism().map_or(1, |n| n.get().min(4))
                } else {
                    t.parse().map_err(|_| "Invalid threads")?
                };
            }
            "--help" => {
                println!("Fourfold RSST constructive research solver\n--input PATH   Rotation text: n, then n rows of degree and cyclic neighbors (zero based)\n--timeout SEC  Maximum 100 seconds\n--json         Print colors and branch counters\n--threads N    1..8 or auto (default 1; original reduction order)\n\nDefault input: 2,048 regions, seed 817. Catalogue reductions and short-circuit\nreconstruction follow RSST sections 3 and 6. The exhaustive locator has no\ncertified quadratic bound. No whole-graph DSATUR fallback.");
                return Ok(true);
            }
            _ => return Err(format!("Unknown argument {arg}")),
        }
    }
    let text = match input {
        Some(path) => fs::read_to_string(path).map_err(|e| e.to_string())?,
        None => include_str!("../../examples/map-2048-817.rotation").into(),
    };
    let text = text
        .lines()
        .map(|l| l.split('#').next().unwrap_or(""))
        .collect::<Vec<_>>()
        .join("\n");
    let mut words = text.split_whitespace();
    let n: usize = words
        .next()
        .ok_or("Missing vertex count")?
        .parse()
        .map_err(|_| "Invalid vertex count")?;
    if n == 0 || n > 16384 {
        return Err("Invalid vertex count".into());
    }
    let mut rotation = Vec::new();
    for _ in 0..n {
        let degree: usize = words
            .next()
            .ok_or("Missing degree")?
            .parse()
            .map_err(|_| "Invalid degree")?;
        if degree >= n {
            return Err("Invalid degree".into());
        }
        let mut row = Vec::new();
        for _ in 0..degree {
            row.push(
                words
                    .next()
                    .ok_or("Missing neighbor")?
                    .parse()
                    .map_err(|_| "Invalid neighbor")?,
            );
        }
        rotation.push(row);
    }
    if words.next().is_some() {
        return Err("Trailing rotation data".into());
    }
    let r = rsst::benchmark_threads(&rotation, timeout * 1000.0, true, threads)?;
    if json {
        println!("{{\"method\":\"rsst-constructive\",\"quadratic_bound_certified\":false,\"backend\":\"rust-native\",\"status\":{:?},\"solverMs\":{},\"threads\":{},\"winningWorker\":{},\"workerMs\":{},\"lowDegreeReductions\":{},\"colors\":{:?},\"message\":{},\"configurations\":{},\"dReductions\":{},\"cReductions\":{},\"separators2to5\":{:?},\"boundaryStates\":{},\"extensionAttempts\":{},\"matchesTested\":{},\"maxDepth\":{},\"triangulationVertices\":{},\"configurationNames\":{:?}}}",r.status,r.solver_ms,r.threads,r.winning_worker.map_or("null".into(),|w|w.to_string()),r.worker_ms,r.counters.low_degree_reductions,r.colors,r.message.as_ref().map_or("null".into(),|m|format!("{m:?}")),r.counters.configurations,r.counters.d_reductions,r.counters.c_reductions,r.counters.separators,r.counters.boundary_states,r.counters.extension_attempts,r.counters.matches_tested,r.counters.max_depth,r.counters.triangulation_vertices,r.counters.configuration_names);
    } else {
        println!("RSST constructive variant: {} in {:.3} ms; {} catalogue reductions; separators 2/3/4/5 {:?}; {} boundary states",r.status,r.solver_ms,r.counters.configurations,r.counters.separators,r.counters.boundary_states);
        if let Some(m) = r.message {
            eprintln!("{m}");
        }
    }
    Ok(r.status == "complete")
}
