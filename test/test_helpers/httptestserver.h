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
	HttpTestServer();
	~HttpTestServer();

	HttpTestServer(const HttpTestServer&) = delete;
	HttpTestServer& operator=(const HttpTestServer&) = delete;

	static HttpTestServer& get_instance();

	std::string get_address();

	// Returns a shared_ptr which will remove the endpoint when it goes out of scope
	std::shared_ptr<void> add_endpoint(const std::string& path,
		std::vector<std::pair<std::string, std::string>> expectedHeaders,
		std::uint16_t status,
		std::vector<std::pair<std::string, std::string>> responseHeaders,
		std::vector<std::uint8_t> body);

private:
	void remove_endpoint(const std::string& mockId);

	InputOutputProcess process;
	std::string address;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_ */
