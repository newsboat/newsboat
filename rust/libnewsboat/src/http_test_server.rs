use httpmock::Method::GET;
use httpmock::{Mock, MockServer};

pub struct MockServerContainer {
    server: MockServer,
}

impl MockServerContainer {
    pub fn new() -> Self {
        let server = MockServer::start();
        Self { server }
    }

    pub fn get_address(&self) -> String {
        self.server.address().to_string()
    }

    pub fn add_endpoint(
        &mut self,
        path: &str,
        body: &[u8],
        expected_headers: &[(&str, &str)],
        status: u16,
        response_headers: &[(&str, &str)],
    ) -> usize {
        let mock = self.server.mock(|when, then| {
            let mut when = when.method(GET).path(path);
            for expected_header in expected_headers {
                when = when.header(expected_header.0, expected_header.1);
            }

            let mut then = then.status(status);
            for response_header in response_headers {
                then = then.header(response_header.0, response_header.1);
            }
            then.body(body);
        });
        mock.id
    }

    pub fn assert_hits(&mut self, mock_id: usize, hits: usize) {
        let mock = Mock::new(mock_id, &self.server);
        mock.assert_hits(hits);
    }
}

impl Default for MockServerContainer {
    fn default() -> Self {
        Self::new()
    }
}
