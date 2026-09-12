use fourfold_engine::benchmark;
use std::{
    env, fs,
    io::{self, Read},
    process,
};

fn parse_graph(text: &str) -> Result<(usize, Vec<u32>), String> {
    let values = text
        .lines()
        .map(|line| line.split('#').next().unwrap_or(""))
        .collect::<Vec<_>>()
        .join("\n");
    let mut words = values.split_whitespace();
    let n: usize = words
        .next()
        .ok_or("Missing vertex count")?
        .parse()
        .map_err(|_| "Invalid vertex count")?;
    let m: usize = words
        .next()
        .ok_or("Missing edge count")?
        .parse()
        .map_err(|_| "Invalid edge count")?;
    if n == 0 || n > 16384 || m > n * 8 {
        return Err("Graph exceeds the demo limits.".into());
    }
    let edges: Vec<u32> = words
        .map(|word| word.parse().map_err(|_| "Invalid edge endpoint"))
        .collect::<Result<_, _>>()?;
    if edges.len() != m * 2 {
        return Err("Declared edge count does not match input.".into());
    }
    Ok((n, edges))
}
fn main() {
    match run() {
        Ok(ok) => {
            if !ok {
                process::exit(2);
            }
        }
        Err(error) => {
            eprintln!("Error: {error}");
            process::exit(1);
        }
    }
}
fn run() -> Result<bool, String> {
    let (mut input, mut method, mut timeout, mut runs, mut json, mut colors) =
        (None, String::from("both"), 100.0, 1usize, false, false);
    let mut args = env::args().skip(1);
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--input" => input = Some(args.next().ok_or("--input needs a path or -")?),
            "--method" => {
                method = args
                    .next()
                    .ok_or("--method needs dsatur, reduction, or both")?
            }
            "--timeout" => {
                timeout = args
                    .next()
                    .ok_or("--timeout needs seconds")?
                    .parse::<f64>()
                    .map_err(|_| "Invalid timeout")?
            }
            "--runs" => {
                runs = args
                    .next()
                    .ok_or("--runs needs a count")?
                    .parse()
                    .map_err(|_| "Invalid run count")?
            }
            "--json" => json = true,
            "--colors" => {
                colors = true;
                json = true;
            }
            "--help" | "-h" => {
                println!("Fourfold native Rust benchmark\n\nDefault: the browser's 2,048-region map, seed 817.\n\n  --input PATH|-        Edge list: n m, then m zero-based endpoint pairs\n  --method METHOD       dsatur, reduction, or both (default)\n  --timeout SECONDS     Per-trial limit, up to 100 (default 100)\n  --runs COUNT          1 to 99 trials (default 1)\n  --json                Write machine-readable results to stdout\n  --colors              Include completed color arrays in JSON\n\nProgress goes to stderr every 5 seconds. Exit 2 means an incomplete trial.\nThese are demo methods; the full 2026 paper algorithm is not implemented.");
                return Ok(true);
            }
            _ => return Err(format!("Unknown argument: {arg}. Use --help.")),
        }
    }
    if runs == 0 || runs > 99 || !timeout.is_finite() || timeout <= 0.0 || timeout > 100.0 {
        return Err("Use 1–99 runs and a timeout greater than 0 and at most 100 seconds.".into());
    }
    let methods: Vec<&str> = match method.as_str() {
        "both" => vec!["dsatur", "reduction"],
        "dsatur" => vec!["dsatur"],
        "reduction" => vec!["reduction"],
        _ => {
            return Err(
                "Choose dsatur, reduction, or both. A full paper solver is not available.".into(),
            )
        }
    };
    let text = match input.as_deref() {
        None => include_str!("../../examples/map-2048-817.edges").to_string(),
        Some("-") => {
            let mut s = String::new();
            io::stdin()
                .read_to_string(&mut s)
                .map_err(|e| e.to_string())?;
            s
        }
        Some(path) => fs::read_to_string(path).map_err(|e| e.to_string())?,
    };
    let (n, edges) = parse_graph(&text)?;
    // Two tiny native warm-ups per method, excluded from measurements.
    for _ in 0..2 {
        for &name in &methods {
            benchmark(3, &[0, 1, 1, 2, 2, 0], name, 1000.0, false)?;
        }
    }
    if !json {
        println!("Rust native · {n} vertices · {} shared borders · {timeout}s per trial\n{:<12} {:>5} {:>13} {:>12}", edges.len()/2, "Method", "Run", "Solver time", "Status");
    }
    let (mut records, mut failed) = (Vec::new(), Vec::new());
    for trial in 1..=runs {
        let mut order = methods.clone();
        if trial % 2 == 0 {
            order.reverse();
        }
        for name in order {
            if failed.contains(&name) {
                continue;
            }
            eprintln!("{name}: trial {trial}/{runs}");
            let result = benchmark(n, &edges, name, timeout * 1000.0, true)?;
            let complete = result.status == "complete";
            if !complete {
                failed.push(name);
            }
            if !json {
                println!(
                    "{name:<12} {trial:>5} {:>10.3} ms {:>12}",
                    result.solver_ms, result.status
                );
            }
            let color_json = if colors && complete {
                format!(",\"colors\":{:?}", result.colors)
            } else {
                String::new()
            };
            records.push(format!("{{\"method\":\"{name}\",\"trial\":{trial},\"status\":\"{}\",\"solverMs\":{},\"validated\":{complete},\"decisions\":{},\"backtracks\":{},\"removed\":{},\"swaps\":{}{color_json}}}", result.status,result.solver_ms,result.decisions,result.backtracks,result.removed,result.swaps));
        }
    }
    if json {
        println!("{{\"backend\":\"rust-native\",\"vertices\":{n},\"edges\":{},\"budgetMs\":{},\"trials\":[{}]}}", edges.len()/2, timeout*1000.0, records.join(","));
    }
    Ok(failed.is_empty())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn edge_list() {
        assert_eq!(
            parse_graph("# triangle\n3 3\n0 1\n1 2\n2 0").unwrap(),
            (3, vec![0, 1, 1, 2, 2, 0])
        );
    }
    #[test]
    fn wrong_count() {
        assert!(parse_graph("3 2\n0 1").is_err());
    }
}
