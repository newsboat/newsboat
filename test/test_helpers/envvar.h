#ifndef NEWSBOAT_TEST_HELPERS_ENVVAR_H_
#define NEWSBOAT_TEST_HELPERS_ENVVAR_H_

#include <functional>
#include <string>

#include "3rd-party/optional.hpp"

namespace test_helpers {

/* \brief Automatically restores environment variable to its original state
 * when the test finishes running.
 *
 * When an EnvVar object is created, it remembers the state of the given
 * environment variable. After that, the test can safely change the variable
 * directly through setenv(3) and unsetenv(3), or via convenience methods set()
 * and unset(). When the test finishes (with whatever result), the EnvVar
 * restores the variable to its original state.
 *
 * If you need to run some code after the variable is changed (e.g. tzset(3)
 * after a change to TZ), use `on_change()` method to specify a function that
 * should be ran. Note that it will be executed only if the variable is changed
 * through set() and unset() methods; EnvVar won't notice a change made using
 * setenv(3), unsetenv(3), or a change made by someone else (e.g. different
 * thread).
 */
class EnvVar {
	std::function<void(nonstd::optional<std::string>)> on_change_fn;
	std::string name;
	std::string value;
	bool was_set = false;

public:
	/// \brief Safekeeps the value of environment variable \a name.
	///
	/// \note Accepts a string by value since you'll probably pass it
	/// a temporary or a string literal anyway. Just do it, and use set() and
	/// unset() methods, instead of keeping a local variable with a name in it
	/// and calling setenv(3) and unsetenv(3).
	///
	/// \note For TZ env var, use `TzEnvVar` instead.
	explicit EnvVar(std::string name_);

	virtual ~EnvVar();

	/// \brief Changes the value of the environment variable.
	///
	/// \note This is equivalent to you calling setenv(3) yourself.
	///
	/// \note This does \emph{not} change the value to which EnvVar will
	/// restore the variable when the test finished running. The variable is
	/// always restored to the state it was in when EnvVar object was
	/// constructed.
	void set(const std::string& new_value) const;

	/// \brief Unsets the environment variable.
	///
	/// \note This does \emph{not} change the value to which EnvVar will
	/// restore the variable when the test finished running. The variable is
	/// always restored to the state it was in when EnvVar object was
	void unset() const;

	/// \brief Specifies a function that should be ran after each call to set()
	/// or unset() methods, and also during object destruction.
	///
	/// In other words, the function will be ran after each change done via or
	/// by this class.
	///
	/// The function is passed an argument which can be either:
	/// - nonstd::nullopt  --  meaning the environment variable is now unset
	/// - std::string      --  meaning the environment variable is now set to
	///                        this value
	void on_change(std::function<void(nonstd::optional<std::string> new_value)> fn);

protected:
	// The main constructor throws for some values of `name`; this one doesn't.
	//
	// This is meant to be used in subclasses that implement behaviors for
	// `name`s that throw. The second parameter is meant to disambiguate the
	// call, and is not used for anything.
	explicit EnvVar(std::string name_, bool /* unused */);
};

/* \brief Save and restore timezone environment variable, calling tzset as
 * appropriate.
 *
 * For details, see the docs for `EnvVar`.
 */
class TzEnvVar final : public EnvVar {
public:
	TzEnvVar();
	virtual ~TzEnvVar() = default;

private:
	// We hide this method because we don't want users to override our handler.
	using EnvVar::on_change;
};

/* \brief Save and restore locale charset environment variable, calling
 * setlocale as appropriate.
 *
 * For details, see the docs for `EnvVar`.
 */
class LcCtypeEnvVar final : public EnvVar {
public:
	LcCtypeEnvVar();
	virtual ~LcCtypeEnvVar() = default;

private:
	// We hide this method because we don't want users to override our handler.
	using EnvVar::on_change;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_ENVVAR_H_ */
