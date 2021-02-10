#ifndef NEWSBOAT_TEST_HELPERS_OPTS_H_
#define NEWSBOAT_TEST_HELPERS_OPTS_H_

#include <memory>
#include <string>
#include <vector>

namespace TestHelpers {

/// Helper class to create argc and argv arguments for CliArgsParser
///
/// When testing CliArgsParser, resource management turned out to be a problem:
/// CliArgsParser requires char** pointing to arguments, but such a pointer
/// can't be easily obtained from any of standard containers. To overcome that,
/// I wrote Opts, which simply copies elements of initializer_list into
/// separate unique_ptr<char>, and presents useful accessors argc() and argv()
/// whose results can be passed right into CliArgsParser. Problem solved!
class Opts {
	/// Individual elements of argv.
	std::vector<std::unique_ptr<char[]>> m_opts;
	/// This is argv as main() knows it.
	std::unique_ptr<char* []> m_data;
	/// This is argc as main() knows it.
	std::size_t m_argc = 0;

public:
	/// Turns \a opts into argc and argv.
	// This constructor is not marked as explicit because it's used in tests
	// that are small and don't care which exact type is being constructed.
	// Making the constructor explicit will only add unnecessary mention of the
	// type name.
	// cppcheck-suppress noExplicitConstructor
	Opts(std::initializer_list<std::string> opts);

	std::size_t argc() const;

	char** argv() const;
};

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_OPTS_H_ */
