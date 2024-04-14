use libnewsboat::http_test_server;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct MockServerContainer(http_test_server::MockServerContainer);

#[cxx::bridge(namespace = "newsboat::http_test_server::bridged")]
mod bridged {
    struct Header {
        key: String,
        value: String,
    }

    extern "Rust" {
        type MockServerContainer;

        fn create() -> Box<MockServerContainer>;
        fn get_address(server: &MockServerContainer) -> String;
        fn add_endpoint(
            server: &mut MockServerContainer,
            path: &str,
            expected_headers: &[Header],
            status: u16,
            response_headers: &[Header],
            body: &[u8],
        ) -> usize;
        fn assert_hits(server: &mut MockServerContainer, mock_id: usize, hits: usize);
    }
}

use bridged::Header;

fn create() -> Box<MockServerContainer> {
    Box::new(MockServerContainer(
        http_test_server::MockServerContainer::new(),
    ))
}

fn get_address(server: &MockServerContainer) -> String {
    server.0.get_address()
}

fn add_endpoint(
    server: &mut MockServerContainer,
    path: &str,
    expected_headers: &[Header],
    status: u16,
    response_headers: &[Header],
    body: &[u8],
) -> usize {
    let expected_headers: Vec<_> = expected_headers
        .iter()
        .map(|h| (h.key.as_str(), h.value.as_str()))
        .collect();
    let response_headers: Vec<_> = response_headers
        .iter()
        .map(|h| (h.key.as_str(), h.value.as_str()))
        .collect();
    server
        .0
        .add_endpoint(path, body, &expected_headers, status, &response_headers)
}

fn assert_hits(server: &mut MockServerContainer, mock_id: usize, hits: usize) {
    server.0.assert_hits(mock_id, hits);
}
