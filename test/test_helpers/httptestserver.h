#ifndef NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_
#define NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_

#include "inputoutputprocess.h"

#include <string>

namespace test_helpers {

class HttpTestServer {
public:
	HttpTestServer();
	~HttpTestServer();

private:
	InputOutputProcess process;
	std::string address;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_HTTPTESTSERVER_H_ */
