#ifndef NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_
#define NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_

#include "inputoutputprocess.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace test_helpers {

class HttpTestServer {
public:
	struct MockRegistration {
		std::shared_ptr<void> lifetime;
		std::string mockId;
	};

	HttpTestServer();
	~HttpTestServer();

	HttpTestServer(const HttpTestServer&) = delete;
	HttpTestServer& operator=(const HttpTestServer&) = delete;

	static HttpTestServer& get_instance();

	std::string get_address();

	// Returns a `MockRegistration` with a lifetime object which will remove the endpoint when it goes out of scope
	MockRegistration add_endpoint(const std::string& path,
		std::vector<std::pair<std::string, std::string>> expectedHeaders,
		std::uint16_t status,
		std::vector<std::pair<std::string, std::string>> responseHeaders,
		std::vector<std::uint8_t> body);

	std::uint32_t num_hits(MockRegistration& mockRegistration);

private:
	void remove_endpoint(const std::string& mockId);

	InputOutputProcess process;
	std::string address;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_ */
