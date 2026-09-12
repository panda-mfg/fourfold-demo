use std::{
    io::Write,
    process::{Command, Output, Stdio},
};

fn run(args: &[&str], input: &str) -> Output {
    let mut child = Command::new(env!("CARGO_BIN_EXE_fourfold"))
        .args(args)
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    child
        .stdin
        .take()
        .unwrap()
        .write_all(input.as_bytes())
        .unwrap();
    child.wait_with_output().unwrap()
}

#[test]
fn stdin_comparison_has_clean_machine_output() {
    let result = run(
        &["--input", "-", "--colors", "--runs", "2"],
        "3 3\n0 1\n1 2\n2 0\n",
    );
    assert!(result.status.success());
    let out = String::from_utf8(result.stdout).unwrap();
    assert_eq!(out.lines().count(), 1);
    assert!(out.starts_with("{\"backend\":\"rust-native\""));
    assert_eq!(out.matches("\"validated\":true").count(), 4);
    assert_eq!(out.matches("\"colors\":[").count(), 4);
    assert!(String::from_utf8(result.stderr)
        .unwrap()
        .contains("trial 2/2"));
}

#[test]
fn duplicate_edges_exit_with_input_error() {
    let result = run(&["--input", "-", "--json"], "3 2\n0 1\n1 0\n");
    assert_eq!(result.status.code(), Some(1));
    assert!(result.stdout.is_empty());
    assert!(String::from_utf8(result.stderr)
        .unwrap()
        .contains("Invalid graph"));
}

#[test]
fn timeout_is_incomplete_and_not_validated() {
    let result = run(
        &["--method", "dsatur", "--timeout", "0.000001", "--colors"],
        "",
    );
    assert_eq!(result.status.code(), Some(2));
    let out = String::from_utf8(result.stdout).unwrap();
    assert!(out.contains("\"status\":\"timeout\""));
    assert!(out.contains("\"validated\":false"));
    assert!(!out.contains("\"colors\":"));
}
