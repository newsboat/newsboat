#include "curlheadercontainer.h"

#include "3rd-party/catch.hpp"
#include "curlhandle.h"

using namespace Newsboat;

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

TEST_CASE("get_header_lines() with given key returns matched header lines (case insensitively)",
	"[CurlHeaderContainer]")
{
	CurlHandle curlHandle;
	CurlHeaderContainerForTesting curlHeaderContainer(curlHandle);

	GIVEN("A few header lines as input") {
		curlHeaderContainer.handle_header("HTTP/1.1 200 OK");
		curlHeaderContainer.handle_header("Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT");
		curlHeaderContainer.handle_header("Cache-Control:no-cache");
		curlHeaderContainer.handle_header("Cache-Control:no-store");
		curlHeaderContainer.handle_header("whitespace-test: \tvalue with spaces\t ");
		curlHeaderContainer.handle_header("");

		WHEN("get_header_lines() is called with a non-existent key") {
			const auto values = curlHeaderContainer.get_header_lines("non-existent");

			THEN("Nothing is returned") {
				REQUIRE(values.size() == 0);
			}
		}

		WHEN("get_header_lines() is called with an existing key") {
			const auto values = curlHeaderContainer.get_header_lines("Cache-Control");

			THEN("all relevant values are returned") {
				REQUIRE(values.size() == 2);
				REQUIRE(values[0] == "no-cache");
				REQUIRE(values[1] == "no-store");
			}
		}

		SECTION("get_header_lines() matches keys case-insensitively") {
			const auto values = curlHeaderContainer.get_header_lines("CACHE-CONTROL");

			REQUIRE(values.size() == 2);
			REQUIRE(values[0] == "no-cache");
			REQUIRE(values[1] == "no-store");
		}

		SECTION("get_header_lines() removes whitespace at start and end of value") {
			const auto values = curlHeaderContainer.get_header_lines("whitespace-test");

			REQUIRE(values.size() == 1);
			REQUIRE(values[0] == "value with spaces");
		}
	}
}
