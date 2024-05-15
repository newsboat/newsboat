#include "httptestserver.h"

#include <stdexcept>
#include <string>

namespace test_helpers {

static HttpTestServer* instance = nullptr;

HttpTestServer::HttpTestServer()
	: process({"./http-test-server"})
{
	address = process.read_line();
	instance = this;
}

HttpTestServer::~HttpTestServer()
{
	process.write_line("exit");
	process.wait_for_exit();
	instance = nullptr;
}

HttpTestServer& HttpTestServer::get_instance()
{
	if (instance == nullptr) {
		throw std::runtime_error("No HttpTestServer instantiated");
	}
	return *instance;
}

std::string HttpTestServer::get_address()
{
	return address;
}

std::shared_ptr<void> HttpTestServer::add_endpoint(const std::string& path,
	std::vector<std::pair<std::string, std::string>> expectedHeaders,
	std::uint16_t status,
	std::vector<std::pair<std::string, std::string>> responseHeaders,
	std::vector<std::uint8_t> body)
{
	process.write_line("add_endpoint");
	process.write_line(path);

	process.write_line(std::to_string(expectedHeaders.size()));
	for (const auto& header : expectedHeaders) {
		process.write_line(header.first);
		process.write_line(header.second);
	}

	process.write_line(std::to_string(status));

	process.write_line(std::to_string(responseHeaders.size()));
	for (const auto& header : responseHeaders) {
		process.write_line(header.first);
		process.write_line(header.second);
	}

	process.write_line(std::to_string(body.size()));
	process.write_binary(body.data(), body.size());

	auto mock_id = process.read_line();

	// Use shared_ptr's custom deleter feature to automatically remove endpoint
	// when the shared_ptr goes out of scope
	return std::shared_ptr<void>(nullptr, [=](void*) {
		remove_endpoint(mock_id);
	});
}

void HttpTestServer::remove_endpoint(const std::string& mockId)
{
	process.write_line("remove_endpoint");
	process.write_line(mockId);
}

} // namespace test_helpers
