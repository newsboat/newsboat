use httpmock::{Method::GET, Mock, MockServer};
use std::io::{Read, stdin};

fn main() {
    let server = MockServer::start();
    let address = server.address().to_string();

    eprintln!("listening on {address}");
    println!("{address}");

    let mut input = String::new();
    loop {
        input.clear();
        let read_result = stdin().read_line(&mut input);
        if let Ok(0) = read_result {
            // Reached eof
            break;
        }
        if let Err(e) = read_result {
            eprintln!("reading failed: {e:?}");
            break;
        }
        let command = input.trim();
        match command {
            "exit" => break,
            "add_endpoint" => add_endpoint(&server),
            "remove_endpoint" => remove_endpoint(&server),
            "num_hits" => num_hits(&server),
            _ => (),
        };
    }
    eprintln!("shutting down http test server");
}

fn read_line() -> String {
    let mut input = String::new();
    stdin().read_line(&mut input).unwrap();
    input.trim().to_owned()
}

fn add_endpoint(server: &MockServer) {
    let path = read_line();

    let num_expected_headers: usize = read_line().parse().unwrap();
    let mut expected_headers = vec![];
    for _ in 0..num_expected_headers {
        let key = read_line();
        let value = read_line();
        expected_headers.push((key, value));
    }

    let status: u16 = read_line().parse().unwrap();

    let num_response_headers: usize = read_line().parse().unwrap();
    let mut response_headers = vec![];
    for _ in 0..num_response_headers {
        let key = read_line();
        let value = read_line();
        response_headers.push((key, value));
    }

    let body_size: usize = read_line().parse().unwrap();
    let mut body = vec![];
    stdin()
        .take(body_size as u64)
        .read_to_end(&mut body)
        .unwrap();

    eprintln!("---------------------------------");
    eprintln!("Adding endpoint {path}");
    eprintln!("Expected Headers: {expected_headers:?}");
    eprintln!("Status: {status}");
    eprintln!("Response headers: {response_headers:?}");
    eprintln!("Body size: {body_size} bytes");
    eprintln!("---------------------------------");

    let mock = server.mock(|when, then| {
        let mut when = when.method(GET).path(path);
        for header in expected_headers {
            when = when.header(header.0, header.1)
        }

        let mut then = then.status(status);
        for header in response_headers {
            then = then.header(header.0, header.1)
        }
        then.body(body);
    });

    println!("{}", mock.id);
}

fn remove_endpoint(server: &MockServer) {
    let mock_id: usize = read_line().parse().unwrap();
    let mut mock = Mock::new(mock_id, server);
    mock.delete();
}

fn num_hits(server: &MockServer) {
    let mock_id: usize = read_line().parse().unwrap();
    let mock = Mock::new(mock_id, server);

    println!("{}", mock.calls());
}
