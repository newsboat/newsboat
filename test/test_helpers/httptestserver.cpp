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

HttpTestServer::MockRegistration HttpTestServer::add_endpoint(const std::string& path,
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

	const auto mockId = process.read_line();

	// Use shared_ptr's custom deleter feature to automatically remove endpoint
	// when the shared_ptr goes out of scope
	const auto lifetime = std::shared_ptr<void>(nullptr, [=](void*) {
		remove_endpoint(mockId);
	});
	return MockRegistration {
		lifetime,
		mockId,
	};
}

void HttpTestServer::remove_endpoint(const std::string& mockId)
{
	process.write_line("remove_endpoint");
	process.write_line(mockId);
}

std::uint32_t HttpTestServer::num_hits(HttpTestServer::MockRegistration& mockRegistration)
{
	process.write_line("num_hits");
	process.write_line(mockRegistration.mockId);

	const auto numHits = process.read_line();
	return std::stoul(numHits);
}

} // namespace test_helpers
