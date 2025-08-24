#include "3rd-party/catch.hpp"

#include "filepath.h"

using namespace newsboat;

TEST_CASE("Can be constructed from a string and converted back into a string",
	"[Filepath]")
{
	const auto example = "/etc/hosts"_path;
	REQUIRE(example.to_locale_string() == "/etc/hosts");
}

TEST_CASE("Can be constructed from non-Unicode data", "[Filepath]")
{
	// See Table 3-7 "Well-Formed UTF-8 Byte Sequences"
	// https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G27506
	const std::string input("/invalid: \x81\x82 but it's fine");
	const auto filepath = Filepath::from_locale_string(input);
	REQUIRE(filepath.to_locale_string() == input);
}

TEST_CASE("Can be displayed even if it contains non-Unicode characters", "[Filepath]")
{
	// See Table 3-7 "Well-Formed UTF-8 Byte Sequences"
	// https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G27506
	const std::string input("/invalid: \x80 but it's fine");
	const auto filepath = Filepath::from_locale_string(input);
	// 0xef 0xbf 0xbd is UTF-8 encoding of U+FFFD REPLACEMENT CHARACTER
	REQUIRE(filepath.display() == "/invalid: \xEF\xBF\xBD but it's fine");
}

TEST_CASE("push() adds a new component to the path", "[Filepath]")
{
	auto dir = "/tmp"_path;

	dir.push("newsboat"_path);
	REQUIRE(dir == "/tmp/newsboat"_path);

	dir.push(".local/share/cache/cache.db"_path);
	REQUIRE(dir == "/tmp/newsboat/.local/share/cache/cache.db"_path);
}

TEST_CASE("push() still adds a separator to non-empty path if new component is empty",
	"[Filepath]")
{
	auto dir = "/root"_path;
	dir.push(""_path);
	REQUIRE(dir.display() == "/root/");
}

TEST_CASE("Can be extended with join()", "[Filepath]")
{
	const auto tmp = "/tmp"_path;

	const auto subdir =
		tmp
		.join("newsboat"_path)
		.join("tests"_path);
	REQUIRE(subdir == "/tmp/newsboat/tests"_path);
}

TEST_CASE("join() still adds a separator to non-empty path if new component is empty",
	"[Filepath]")
{
	const auto path = "relative path"_path;
	const auto path_with_trailing_slash = path.join(Filepath{});
	REQUIRE(path_with_trailing_slash.display() == "relative path/");
}

TEST_CASE("Can be copied", "[Filepath]")
{
	auto original = "/etc/hosts"_path;

	const auto check = [&original](const Filepath& copy) {
		REQUIRE(original == copy);
		REQUIRE(original.display() == "/etc/hosts");

		// Demonstrate that changing the original object doesn't modify the copy
		original.push(" a bit more"_path);
		REQUIRE(original.display() == "/etc/hosts/ a bit more");
		REQUIRE(copy.display() == "/etc/hosts");
	};

	SECTION("with copy constructor") {
		const Filepath copy(original);
		check(copy);
	}

	SECTION("with copy assignment operator") {
		const Filepath copy = original;
		check(copy);
	}
}

TEST_CASE("Can't set extension for an empty path", "[Filepath]")
{
	Filepath path;
	REQUIRE_FALSE(path.set_extension("exe"));
}

TEST_CASE("Can set extension for non-empty path", "[Filepath]")
{
	auto path = "file"_path;

	SECTION("extension is UTF-8") {
		REQUIRE(path.set_extension("exe"));
		REQUIRE(path == "file.exe"_path);
	}

	SECTION("extension is not a valid UTF-8 string") {
		REQUIRE(path.set_extension("\x80"));
		REQUIRE(path == "file.\x80"_path);
	}
}

TEST_CASE("add_extension passes tests from Rust docs", "[Filepath]")
{
	// Copied from https://doc.rust-lang.org/std/path/struct.PathBuf.html#method.add_extension

	auto path = "/feel/the"_path;

	REQUIRE(path.add_extension("formatted"));
	REQUIRE("/feel/the.formatted"_path == path);

	REQUIRE(path.add_extension("dark.side"));
	REQUIRE("/feel/the.formatted.dark.side"_path == path);

	REQUIRE(path.set_extension("cookie"));
	REQUIRE("/feel/the.formatted.dark.cookie"_path == path);

	REQUIRE(path.set_extension(""));
	REQUIRE("/feel/the.formatted.dark"_path == path);

	REQUIRE(path.add_extension(""));
	REQUIRE("/feel/the.formatted.dark"_path == path);
}

TEST_CASE("Can check if path is absolute", "[Filepath]")
{
	Filepath path;
	SECTION("empty path is not absolute") {
		REQUIRE_FALSE(path.is_absolute());
	}

	SECTION("path that starts with a slash is absolute") {
		path.push("/etc"_path);
		REQUIRE(path.display() == "/etc");
		REQUIRE(path.is_absolute());

		path.push("ca-certificates"_path);
		REQUIRE(path.display() == "/etc/ca-certificates");
		REQUIRE(path.is_absolute());
	}

	SECTION("path that doesn't start with a slash is not absolute") {
		path.push("vmlinuz"_path);
		REQUIRE(path.display() == "vmlinuz");
		REQUIRE_FALSE(path.is_absolute());

		path.push("undefined"_path);
		REQUIRE(path.display() == "vmlinuz/undefined");
		REQUIRE_FALSE(path.is_absolute());
	}
}

TEST_CASE("Can check if path starts with a given base path", "[Filepath]")
{
	SECTION("Empty path") {
		const Filepath path;

		SECTION("Base path is empty") {
			REQUIRE(path.starts_with(Filepath{}));
		}

		SECTION("Base path is not empty") {
			REQUIRE_FALSE(path.starts_with("/etc"_path));
		}
	}

	SECTION("Non-empty path that doesn't start with the base") {
		const auto path = "/etcetera"_path;
		const auto base = "/etc"_path;
		REQUIRE_FALSE(path.starts_with(base));
	}

	SECTION("Non-empty path that starts with the base") {
		const auto path = "/usr/local/bin/newsboat"_path;
		const auto base = "/usr/local"_path;
		REQUIRE(path.starts_with(base));
	}

	SECTION("Base is not a valid UTF-8 string") {
		const auto path = "/test\x81\x82/foobar"_path;

		SECTION("Path doesn't start with base") {
			const auto base = "baz\x80quux"_path;
			REQUIRE_FALSE(path.starts_with(base));
		}

		SECTION("Path starts with base") {
			const auto base = "/test\x81\x82/"_path;
			REQUIRE(path.starts_with(base));
		}
	}
}

TEST_CASE("Can extract the final component of the path (file or directory name)",
	"[Filepath]")
{
	SECTION("Empty path has no final component") {
		const Filepath path;
		REQUIRE(path.file_name() == std::nullopt);
	}

	SECTION("The final component of a path with single level is the path itself") {
		const auto path = "hello"_path;
		REQUIRE(path.file_name().value() == path);
	}

	SECTION("Multi-level path") {
		const auto path = "/dev/pts/0"_path;
		REQUIRE(path.file_name().value() == "0"_path);
	}

	SECTION("Final component is not a valid UTF-8 string") {
		const auto path = "/whatever/one\x80two"_path;
		REQUIRE(path.file_name().value() == "one\x80two"_path);
	}
}

TEST_CASE("Can be ordered lexicographically", "[Filepath]")
{
	const auto root = "/"_path;
	const auto var_log = "/var/log"_path;
	const auto home_minoru = "/home/minoru"_path;
	const auto home_minoru_src_newsboat = "/home/minoru/src/newsboat"_path;

	SECTION("operator<") {
		SECTION("Path to directory is less than the path to its subdirectory") {
			REQUIRE(root < var_log);
			REQUIRE(root < home_minoru);
			REQUIRE(home_minoru < home_minoru_src_newsboat);

			REQUIRE_FALSE(home_minoru_src_newsboat < root);
		}

		SECTION("Disparate paths are ordered lexicographically") {
			REQUIRE(home_minoru < var_log);
			REQUIRE(home_minoru_src_newsboat < var_log);

			REQUIRE_FALSE(home_minoru_src_newsboat < home_minoru);
		}
	}

	SECTION("operator>") {
		SECTION("Path to subdirectory is greater than the path to its parent directory") {
			REQUIRE(var_log > root);
			REQUIRE(home_minoru > root);
			REQUIRE(home_minoru_src_newsboat > home_minoru);

			REQUIRE_FALSE(root > home_minoru_src_newsboat);
		}

		SECTION("Disparate paths are ordered lexicographically") {
			REQUIRE(var_log > home_minoru);
			REQUIRE(var_log > home_minoru_src_newsboat);

			REQUIRE_FALSE(home_minoru > home_minoru_src_newsboat);
		}
	}

	SECTION("operator<=") {
		SECTION("Any path is less than or equal to itself") {
			REQUIRE(root <= root);
			REQUIRE(var_log <= var_log);
			REQUIRE(home_minoru <= home_minoru);
			REQUIRE(home_minoru_src_newsboat <= home_minoru_src_newsboat);
		}

		SECTION("Path to directory is less than or equal to the path to its subdirectory") {
			REQUIRE(root <= var_log);
			REQUIRE(root <= home_minoru);
			REQUIRE(home_minoru <= home_minoru_src_newsboat);

			REQUIRE_FALSE(home_minoru_src_newsboat <= root);
		}

		SECTION("Disparate paths are ordered lexicographically") {
			REQUIRE(home_minoru <= var_log);
			REQUIRE(home_minoru_src_newsboat <= var_log);

			REQUIRE_FALSE(home_minoru_src_newsboat <= home_minoru);
		}
	}

	SECTION("operator>=") {
		SECTION("Any path is greater than or equal to itself") {
			REQUIRE(root >= root);
			REQUIRE(var_log >= var_log);
			REQUIRE(home_minoru >= home_minoru);
			REQUIRE(home_minoru_src_newsboat >= home_minoru_src_newsboat);
		}

		SECTION("Path to subdirectory is greater than or equal to the path to its parent directory") {
			REQUIRE(var_log >= root);
			REQUIRE(home_minoru >= root);
			REQUIRE(home_minoru_src_newsboat >= home_minoru);

			REQUIRE_FALSE(root >= home_minoru_src_newsboat);
		}

		SECTION("Disparate paths are ordered lexicographically") {
			REQUIRE(var_log >= home_minoru);
			REQUIRE(var_log >= home_minoru_src_newsboat);

			REQUIRE_FALSE(home_minoru >= home_minoru_src_newsboat);
		}
	}
}

TEST_CASE("Can be constructed from a literal with user-defined literal syntax",
	"[Filepath]")
{
	const auto example = "/etc/hosts"_path;
	REQUIRE(example.to_locale_string() == "/etc/hosts");
}
