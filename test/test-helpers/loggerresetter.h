#ifndef NEWSBOAT_TEST_HELPERS_LOGGERRESETTER_H_
#define NEWSBOAT_TEST_HELPERS_LOGGERRESETTER_H_

namespace TestHelpers {

// Upon construction *and* upon destruction, sets empty log file and "NONE"
// logging level. Create this object early in your test to ensure that it runs
// with logging disabled, or to ensure a known-good starting state for your own
// logging settings.
class LoggerResetter {
public:
	LoggerResetter();
	~LoggerResetter();
};

} /* namespace TestHelpers */

#endif /* NEWSBOAT_TEST_HELPERS_LOGGERRESETTER_H_ */
