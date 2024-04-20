#include "httptestserver.h"

namespace test_helpers {

HttpTestServer::HttpTestServer()
	: process({"./http-test-server"})
{
	address = process.read_line();
}

HttpTestServer::~HttpTestServer()
{
	process.write_line("exit");
	process.wait_for_exit();
}

} // namespace test_helpers
