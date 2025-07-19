#include "remoteapi.h"

#include <memory>

#include "3rd-party/catch.hpp"

#include "configcontainer.h"
#include "configparser.h"

using namespace newsboat;

/*
 * Mock class to be able to access `user` and `pass` after running
 * `get_credentials()`.
 */
class test_api : public RemoteApi {
public:
	explicit test_api(ConfigContainer& c)
		: RemoteApi(c)
	{
	}
	std::string get_user(const std::string& scope, const std::string& name)
	{
		return get_credentials(scope, name).user;
	}
	std::string get_pass(const std::string& scope, const std::string& name)
	{
		return get_credentials(scope, name).pass;
	}
	bool authenticate() override
	{
		throw 0;
	}
	std::vector<TaggedFeedUrl> get_subscribed_urls() override
	{
		throw 0;
	}
	void add_custom_headers(curl_slist**) override
	{
		throw 0;
	}
	bool mark_all_read(const std::string&) override
	{
		throw 0;
	}
	bool mark_article_read(const std::string&, bool) override
	{
		throw 0;
	}
	bool update_article_flags(const std::string&,
		const std::string&,
		const std::string&) override
	{
		throw 0;
	}
};

TEST_CASE("get_credentials() returns the users name and password",
	"[RemoteApi]")
{
	ConfigContainer cfg;
	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);
	cfgparser.parse_file(Filepath::from_locale_string("data/test-config-credentials.txt"));
	auto api = std::make_unique<test_api>(cfg);
	REQUIRE(api->get_user("ttrss", "") == "ttrss-user");
	REQUIRE(api->get_pass("ttrss", "") == "my-birthday");
	REQUIRE(api->get_pass("ocnews", "") == "dadada");
	REQUIRE(api->get_pass("oldreader", "") == "123456");
	REQUIRE(api->get_user("miniflux", "") == "miniflux-user");
	REQUIRE(api->get_pass("miniflux", "") == "securepassw0rd");
	REQUIRE(api->get_user("feedbin", "") == "feedbin-user");
	REQUIRE(api->get_pass("feedbin", "") == "feedbin-pass");
	// test cases that would raise a prompt and ask for username or password
	// can not be covered at the moment and would require a redesign of this
	// method with a sole purpose to make it testable.
	// see:
	// https://github.com/akrennmair/newsbeuter/pull/503#issuecomment-282491118
	//
	// following two tests will show the prompt and block tests
	// REQUIRE(api->get_user("newsblur", "Newsblur") == "user");
	// REQUIRE(api->get_pass("newsblur", "Newsblur") == "secret");
}

TEST_CASE("read_password() returns the first line of the file", "[RemoteApi]")
{
	REQUIRE(RemoteApi::read_password(Filepath::from_locale_string("/dev/null")) == "");
	REQUIRE_NOTHROW(RemoteApi::read_password(
			Filepath::from_locale_string("a-passwordfile-that-is-guaranteed-to-not-exist.txt")));
	REQUIRE(RemoteApi::read_password(
			Filepath::from_locale_string("a-passwordfile-that-is-guaranteed-to-not-exist.txt")) == "");
	REQUIRE(RemoteApi::read_password(
			Filepath::from_locale_string("data/single-line-string.txt")) == "single line with spaces");
	REQUIRE(RemoteApi::read_password(
			Filepath::from_locale_string("data/multi-line-string.txt")) == "string with spaces");
}

TEST_CASE("eval_password() returns the first line of command's output",
	"[RemoteApi]")
{
	REQUIRE(RemoteApi::eval_password("echo ''") == "");
	REQUIRE(RemoteApi::eval_password("echo 'aaa'") == "aaa");
	REQUIRE(RemoteApi::eval_password("echo 'aaa\naaa'") == "aaa");
	REQUIRE(RemoteApi::eval_password("echo 'aaa\n\raaa'") == "aaa");
	REQUIRE(RemoteApi::eval_password("echo 'aaa\raaa'") == "aaa");
	REQUIRE(RemoteApi::eval_password("echo ' aaa \naaa'") == " aaa ");
	REQUIRE(RemoteApi::eval_password("echo '\naaa'") == "");
	REQUIRE_NOTHROW(RemoteApi::eval_password(
			"a-program-that-is-guaranteed-to-not-exists"));
	REQUIRE(RemoteApi::eval_password(
			"a-program-that-is-guaranteed-to-not-exists") == "");
	REQUIRE_NOTHROW(
		RemoteApi::eval_password("printf 'string with no newline'"));
	REQUIRE(RemoteApi::eval_password("printf 'string with no newline'") ==
		"string with no newline");
	REQUIRE(RemoteApi::eval_password("echo 'a'; exit 1;") == "a");
	REQUIRE_NOTHROW(RemoteApi::eval_password("echo 'a'; exit 1;"));
	REQUIRE(RemoteApi::eval_password("(>&2 echo 'b'); echo 'a'") == "a");
	// test cases that would require user input can not be covered at the
	// moment and would require a redesign of this method with a sole
	// purpose to make it testable. see:
	// https://github.com/akrennmair/newsbeuter/pull/503#issuecomment-282491118
	//
	// following test will wait for user input and block tests
	// REQUIRE(RemoteApi->eval_password("read password") == "");
}
