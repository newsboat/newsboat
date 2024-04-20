use httpmock::MockServer;
use std::io::stdin;

fn main() {
    let server = MockServer::start();
    let address = server.address().to_string();

    eprintln!("listening on {}", address);
    println!("{}", address);

    let mut input = String::new();
    loop {
        input.clear();
        let read_result = stdin().read_line(&mut input);
        if let Ok(0) = read_result {
            // Reached eof
            break;
        }
        if let Err(e) = read_result {
            eprintln!("reading failed: {:?}", e);
            break;
        }
        let command = input.trim();
        match command {
            "exit" => break,
            _ => (),
        };
    }
    eprintln!("shutting down http test server");
}
