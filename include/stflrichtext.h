#ifndef NEWSBOAT_STFLSTRING_H_
#define NEWSBOAT_STFLSTRING_H_

#include <map>
#include <string>

namespace newsboat {

class StflRichText {
public:
	static StflRichText from_plaintext_string(std::string);
	static StflRichText from_quoted(std::string);

	StflRichText(const StflRichText&) = default;
	StflRichText(StflRichText&&) = default;
	StflRichText& operator=(const StflRichText&) = default;
	StflRichText& operator=(StflRichText&&) = default;
	~StflRichText() = default;

	friend StflRichText operator+(StflRichText left, const StflRichText& right);

	void apply_style_tag(const std::string& tag, size_t start, size_t end);

	std::string get_plaintext() const;
	std::string stfl_quoted_string() const;

private:
	StflRichText(std::string&&,
		std::map<size_t, std::string>&&); // Only constructable using the public static functions
	static std::map<size_t, std::string> extract_style_tags(std::string&);
	static void merge_style_tag(std::map<size_t, std::string>& tags, const std::string& tag,
		size_t start, size_t end);
	static std::string insert_style_tags(const std::string& str,
		const std::map<size_t, std::string>& tags);

	std::string text; // plaintext string (without highlighting)
	std::map<size_t, std::string> style_tags;
};

} // namespace newsboat

#endif /* NEWSBOAT_STFLSTRING_H_ */
