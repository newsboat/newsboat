use libNewsboat::utils::run_program;
use std::thread::{sleep, spawn};

#[test]
fn t_run_program_works_for_large_inputs() {
    // 10 characters repeated 100k times is about 1 megabyte of text
    let large_input = "helloworld".repeat(100_000);
    let runner = {
        let input = large_input.clone();
        spawn(move || run_program(&["sh", "-c", "cat"], input))
    };
    // cat should be able to process 1MB of input in under a second
    sleep(std::time::Duration::from_secs(1));
    assert!(runner.is_finished());
    let result = runner.join().expect("Runner thread panicked");
    assert_eq!(result, large_input);
}
