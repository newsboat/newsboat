#include "statusline.h"

#include "3rd-party/catch.hpp"

using namespace Newsboat;

class StatusTester : public IStatus {
public:
	StatusTester()
		: last_string_was_error(false)
	{
	}
	void set_status(const std::string& message) override
	{
		last_message = message;
		last_string_was_error = false;
	}
	void show_error(const std::string& message) override
	{
		last_message = message;
		last_string_was_error = true;
	}

	std::string last_message;
	bool last_string_was_error;
};

TEST_CASE("StatusLine passes status messages to registered IStatus implementation",
	"[StatusLine]")
{
	StatusTester status_message_handler;
	StatusLine status_line(status_message_handler);

	GIVEN("no previous registered messages") {
		WHEN("show_message() is called") {
			status_line.show_message("this is a status");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is a status");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);
			}
		}

		WHEN("show_error() is called") {
			status_line.show_error("this is an error");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is an error");
				REQUIRE(status_message_handler.last_string_was_error);
			}
		}

		WHEN("show_message_until_finished() is called") {
			auto message_lifetime = status_line.show_message_until_finished("test message...");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "test message...");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);
			}

			WHEN("the message_lifetime object goes out of scope") {
				message_lifetime.reset();

				THEN("the message is cleared") {
					REQUIRE(status_message_handler.last_message == "");
					REQUIRE_FALSE(status_message_handler.last_string_was_error);
				}
			}
		}
	}

	GIVEN("a previous message or error") {
		status_line.show_message("regular status");
		status_line.show_error("error message");

		WHEN("show_message() is called") {
			status_line.show_message("this is a status");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is a status");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);
			}
		}

		WHEN("show_error() is called") {
			status_line.show_error("this is an error");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is an error");
				REQUIRE(status_message_handler.last_string_was_error);
			}
		}

		WHEN("show_message_until_finished() is called") {
			auto message_lifetime = status_line.show_message_until_finished("test message...");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "test message...");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);
			}

			WHEN("the message_lifetime object goes out of scope") {
				message_lifetime.reset();

				THEN("the message is cleared") {
					REQUIRE(status_message_handler.last_message == "");
					REQUIRE_FALSE(status_message_handler.last_string_was_error);
				}
			}
		}
	}

	GIVEN("a previous show_message_until_finished call") {
		auto previous_message_lifetime =
			status_line.show_message_until_finished("previous message...");

		WHEN("show_message() is called") {
			status_line.show_message("this is a status");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is a status");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);

				WHEN("the previous show_message_until_finished() lifetime goes away") {
					previous_message_lifetime.reset();

					THEN("the message is not overwritten") {
						REQUIRE(status_message_handler.last_message == "this is a status");
						REQUIRE_FALSE(status_message_handler.last_string_was_error);
					}
				}
			}
		}

		WHEN("show_error() is called") {
			status_line.show_error("this is an error");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "this is an error");
				REQUIRE(status_message_handler.last_string_was_error);

				WHEN("the previous show_message_until_finished() lifetime goes away") {
					previous_message_lifetime.reset();

					THEN("the message is not overwritten") {
						REQUIRE(status_message_handler.last_message == "this is an error");
						REQUIRE(status_message_handler.last_string_was_error);
					}
				}
			}
		}

		WHEN("show_message_until_finished() is called") {
			auto message_lifetime = status_line.show_message_until_finished("test message...");

			THEN("the message is passed to the IStatus implementation") {
				REQUIRE(status_message_handler.last_message == "test message...");
				REQUIRE_FALSE(status_message_handler.last_string_was_error);
			}

			WHEN("the message_lifetime object goes out of scope") {
				message_lifetime.reset();

				THEN("the previous message is restored") {
					REQUIRE(status_message_handler.last_message == "previous message...");
					REQUIRE_FALSE(status_message_handler.last_string_was_error);
				}
			}
		}
	}
}

TEST_CASE("show_message_until_finished() will restore old message when lifetime is destructed",
	"[StatusLine]")
{
	StatusTester status_message_handler;
	StatusLine status_line(status_message_handler);

	GIVEN("multiple message lifetimes") {
		auto message_lifetime1 = status_line.show_message_until_finished("first message");
		REQUIRE(status_message_handler.last_message == "first message");
		auto message_lifetime2 = status_line.show_message_until_finished("second message");
		REQUIRE(status_message_handler.last_message == "second message");
		auto message_lifetime3 = status_line.show_message_until_finished("third message");
		REQUIRE(status_message_handler.last_message == "third message");

		WHEN("the latest message goes out of scope") {
			message_lifetime3.reset();

			THEN("the most recent active status message is restored") {
				REQUIRE(status_message_handler.last_message == "second message");

				THEN("a further destruction causes the then latest message to be restored") {
					message_lifetime2.reset();
					REQUIRE(status_message_handler.last_message == "first message");

					THEN("the last destruction causes the status line to be cleared") {
						message_lifetime1.reset();
						REQUIRE(status_message_handler.last_message == "");
					}
				}
			}
		}

		WHEN("an older message goes out of scope") {
			status_message_handler.last_message = "should not be overwritten";
			message_lifetime2.reset();
			message_lifetime1.reset();

			THEN("no changes are applied to the status line") {
				REQUIRE(status_message_handler.last_message == "should not be overwritten");
			}
		}

		WHEN("messages go out of scope in non-sorted mode") {
			message_lifetime2.reset();

			THEN("still, the most recent message is shown every time") {
				REQUIRE(status_message_handler.last_message == "third message");
				message_lifetime3.reset();
				REQUIRE(status_message_handler.last_message == "first message");
				message_lifetime1.reset();
				REQUIRE(status_message_handler.last_message == "");
			}
		}
	}
}
