#ifndef NEWSBOAT_STFLPP_H_
#define NEWSBOAT_STFLPP_H_

extern "C" {
#include <stfl.h>
}

#include <string>

namespace Newsboat {

class Stfl {
public:
	class Form {
	public:
		explicit Form(const std::string& text);
		~Form();

		// Make sure this class cannot accidentally get copied.
		// Copying this class would be bad because the `f` and `ipool` member
		// variables are managed manually.
		Form(const Form&) = delete;
		Form& operator=(const Form&) = delete;
		// The move operations are not needed at the moment but can be added when necessary.
		Form(Form&&) = delete;
		Form& operator=(Form&&) = delete;

		std::string run(int timeout);

		std::string get(const std::string& name);
		void set(const std::string& name, const std::string& value);

		std::string get_focus();
		void set_focus(const std::string& name);

		std::string dump(const std::string& name,
			const std::string& prefix,
			int focus);
		void modify(const std::string& name,
			const std::string& mode,
			const std::string& text);

	private:
		stfl_form* f;
		stfl_ipool* ipool;
	};

	static void reset();
	static std::string quote(const std::string& text);
};

} // namespace Newsboat

#endif /* NEWSBOAT_STFLPP_H_ */
