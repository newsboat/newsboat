#ifndef NEWSBOAT_STFLPP_H_
#define NEWSBOAT_STFLPP_H_

extern "C" {
#include <stfl.h>
}

#include "utf8string.h"

namespace newsboat {

namespace Stfl {

class Form {
public:
	explicit Form(const Utf8String& text);
	~Form();

	// Make sure this class cannot accidentally get copied.
	// Copying this class would be bad because the `f` and `ipool` member
	// variables are managed manually.
	Form(const Form&) = delete;
	Form& operator=(const Form&) = delete;
	// The move operations are not needed at the moment but can be added when necessary.
	Form(Form&&) = delete;
	Form& operator=(Form&&) = delete;

	const char* run(int timeout);

	Utf8String get(const Utf8String& name);
	void set(const Utf8String& name, const Utf8String& value);

	Utf8String get_focus();
	void set_focus(const Utf8String& name);

	Utf8String dump(const Utf8String& name,
		const Utf8String& prefix,
		int focus);
	void modify(const Utf8String& name,
		const Utf8String& mode,
		const Utf8String& text);

private:
	stfl_form* f;
	stfl_ipool* ipool;
};

void reset();
Utf8String quote(const Utf8String& text);

} // namespace Stfl

} // namespace newsboat

#endif /* NEWSBOAT_STFLPP_H_ */
