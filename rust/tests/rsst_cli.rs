use std::process::Command;
#[test]
fn native_rsst_threads_and_clean_json() {
    let fixture = format!(
        "{}/../examples/icosahedron.rotation",
        env!("CARGO_MANIFEST_DIR")
    );
    for threads in ["1", "2", "4", "8"] {
        let r = Command::new(env!("CARGO_BIN_EXE_fourfold-rsst"))
            .args(["--input", &fixture, "--threads", threads, "--json"])
            .output()
            .unwrap();
        assert!(r.status.success(), "{}", String::from_utf8_lossy(&r.stderr));
        let out = String::from_utf8(r.stdout).unwrap();
        assert_eq!(out.lines().count(), 1);
        assert!(out.contains("\"status\":\"complete\""));
        assert!(out.contains(&format!("\"threads\":{threads}")));
        assert!(out.contains("\"quadratic_bound_certified\":false"));
        assert!(!out.contains("\"winningWorker\":null"));
    }
    for threads in ["0", "9"] {
        assert_eq!(
            Command::new(env!("CARGO_BIN_EXE_fourfold-rsst"))
                .args(["--threads", threads])
                .output()
                .unwrap()
                .status
                .code(),
            Some(1)
        );
    }
}
#[test]
fn rsst_timeout_is_incomplete() {
    let r = Command::new(env!("CARGO_BIN_EXE_fourfold-rsst"))
        .args(["--timeout", "0.000001", "--threads", "4", "--json"])
        .output()
        .unwrap();
    assert_eq!(r.status.code(), Some(2));
    let out = String::from_utf8(r.stdout).unwrap();
    assert!(out.contains("\"status\":\"timeout\""));
    assert!(out.contains("\"colors\":[]"));
}
