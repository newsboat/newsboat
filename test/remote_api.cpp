#include "remote_api.h"

#include <memory>

#include "3rd-party/catch.hpp"

using namespace newsboat;

/*
 * Mock class to be able to access `user` and `pass` after running
 * `get_credentials()`.
 */
class test_api : public remote_api {
public:
	test_api(configcontainer* c)
		: remote_api(c)
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
	bool authenticate()
	{
		throw 0;
	}
	std::vector<tagged_feedurl> get_subscribed_urls()
	{
		throw 0;
	}
	void add_custom_headers(curl_slist**)
	{
		throw 0;
	}
	bool mark_all_read(const std::string&)
	{
		throw 0;
	}
	bool mark_article_read(const std::string&, bool)
	{
		throw 0;
	}
	bool update_article_flags(
		const std::string&,
		const std::string&,
		const std::string&)
	{
		throw 0;
	}
};

TEST_CASE(
	"get_credentials() returns the users name and password",
	"[remote_api]")
{
	configcontainer cfg;
	configparser cfgparser;
	cfg.register_commands(cfgparser);
	cfgparser.parse("data/test-config-credentials.txt");
	std::unique_ptr<test_api> api(new test_api(&cfg));
	REQUIRE(api->get_user("ttrss", "") == "ttrss-user");
	REQUIRE(api->get_pass("ttrss", "") == "my-birthday");
	REQUIRE(api->get_pass("ocnews", "") == "dadada");
	REQUIRE(api->get_pass("oldreader", "") == "123456");
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

TEST_CASE("read_password() returns the first line of the file", "[remote_api]")
{
	REQUIRE(remote_api::read_password("/dev/null") == "");
	REQUIRE_NOTHROW(remote_api::read_password(
		"a-passwordfile-that-is-guaranteed-to-not-exist.txt"));
	REQUIRE(remote_api::read_password(
			"a-passwordfile-that-is-guaranteed-to-not-exist.txt")
		== "");
	REQUIRE(remote_api::read_password("data/single-line-string.txt")
		== "single line with spaces");
	REQUIRE(remote_api::read_password("data/multi-line-string.txt")
		== "string with spaces");
}

TEST_CASE(
	"eval_password() returns the first line of command's output",
	"[remote_api]")
{
	REQUIRE(remote_api::eval_password("echo ''") == "");
	REQUIRE(remote_api::eval_password("echo 'aaa'") == "aaa");
	REQUIRE(remote_api::eval_password("echo 'aaa\naaa'") == "aaa");
	REQUIRE(remote_api::eval_password("echo 'aaa\n\raaa'") == "aaa");
	REQUIRE(remote_api::eval_password("echo 'aaa\raaa'") == "aaa");
	REQUIRE(remote_api::eval_password("echo ' aaa \naaa'") == " aaa ");
	REQUIRE(remote_api::eval_password("echo '\naaa'") == "");
	REQUIRE_NOTHROW(remote_api::eval_password(
		"a-program-that-is-guaranteed-to-not-exists"));
	REQUIRE(remote_api::eval_password(
			"a-program-that-is-guaranteed-to-not-exists")
		== "");
	REQUIRE_NOTHROW(
		remote_api::eval_password("printf 'string with no newline'"));
	REQUIRE(remote_api::eval_password("printf 'string with no newline'")
		== "string with no newline");
	REQUIRE(remote_api::eval_password("echo 'a'; exit 1;") == "a");
	REQUIRE_NOTHROW(remote_api::eval_password("echo 'a'; exit 1;"));
	REQUIRE(remote_api::eval_password("(>&2 echo 'b'); echo 'a'") == "a");
	// test cases that would require user input can not be covered at the
	// moment and would require a redesign of this method with a sole
	// purpose to make it testable. see:
	// https://github.com/akrennmair/newsbeuter/pull/503#issuecomment-282491118
	//
	// following test will wait for user input and block tests
	// REQUIRE(remote_api->eval_password("read password") == "");
}
