#ifndef NEWSBOAT_STFLSTRING_H_
#define NEWSBOAT_STFLSTRING_H_

#include <string>

#include "libnewsboat-ffi/src/stflrichtext.rs.h" // IWYU pragma: export

namespace newsboat {

class StflRichText {
public:
	static StflRichText from_plaintext(const std::string& text);
	static StflRichText from_plaintext_with_style(const std::string& text,
		const std::string& style_tag);
	static StflRichText from_quoted(const std::string& text);

	StflRichText(const StflRichText&);
	StflRichText(StflRichText&&) = default;
	StflRichText& operator=(const StflRichText&);
	StflRichText& operator=(StflRichText&&) = default;
	~StflRichText() = default;

	void append(const StflRichText& other);
	void highlight_searchphrase(const std::string& search, bool case_insensitive = true);
	void apply_style_tag(const std::string& tag, size_t start, size_t end);

	std::string plaintext() const;
	std::string stfl_quoted() const;

private:
	rust::Box<stflrichtext::bridged::StflRichText> rs_object;

	// Only constructable using the public static functions
	explicit StflRichText(rust::Box<stflrichtext::bridged::StflRichText>&&);
};

} // namespace newsboat

#endif /* NEWSBOAT_STFLSTRING_H_ */
