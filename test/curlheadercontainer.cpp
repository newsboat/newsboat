#include "curlheadercontainer.h"

#include "3rd-party/catch.hpp"
#include "curlhandle.h"

using namespace newsboat;

class CurlHeaderContainerForTesting : public CurlHeaderContainer {
public:
	CurlHeaderContainerForTesting(CurlHandle& curlHandle)
		: CurlHeaderContainer(curlHandle)
	{
	}

	using CurlHeaderContainer::handle_header;
};

TEST_CASE("CurlHeaderContainer::handle_header() stores incoming headers",
	"[CurlHeaderContainer]")
{
	CurlHandle curlHandle;

	GIVEN("A CurlHeaderContainer") {
		CurlHeaderContainerForTesting curlHeaderContainer(curlHandle);

		WHEN("handle_header() is called repeatedly") {
			curlHeaderContainer.handle_header("first header");
			curlHeaderContainer.handle_header("location: test");
			curlHeaderContainer.handle_header("");

			THEN("all headers can be retrieved") {
				const auto headers = curlHeaderContainer.get_header_lines();

				REQUIRE(headers.size() == 3);
				REQUIRE(headers[0] == "first header");
				REQUIRE(headers[1] == "location: test");
				REQUIRE(headers[2] == "");
			}
		}
	}
}

TEST_CASE("CurlHeaderContainer::handle_header() discards previous headers when receiving an HTTP status line",
	"[CurlHeaderContainer]")
{
	CurlHandle curlHandle;

	GIVEN("A CurlHeaderContainer") {
		CurlHeaderContainerForTesting curlHeaderContainer(curlHandle);

		WHEN("handle_header() is called repeatedly with an HTTP status line in between") {
			curlHeaderContainer.handle_header("HTTP/1.1 301 Moved Permanently");
			curlHeaderContainer.handle_header("header of first response");
			curlHeaderContainer.handle_header("location: somewhere else");
			curlHeaderContainer.handle_header("");
			curlHeaderContainer.handle_header("HTTP/1.1 200 OK");
			curlHeaderContainer.handle_header("header of second response");
			curlHeaderContainer.handle_header("");

			THEN("all headers can be retrieved") {
				const auto headers = curlHeaderContainer.get_header_lines();

				REQUIRE(headers.size() == 3);
				REQUIRE(headers[0] == "HTTP/1.1 200 OK");
				REQUIRE(headers[1] == "header of second response");
				REQUIRE(headers[2] == "");
			}
		}
	}
}
