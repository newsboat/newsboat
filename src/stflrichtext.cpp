#include "stflrichtext.h"

#include <utility>

#include "utils.h"

namespace newsboat {

StflRichText::StflRichText(std::string&& text, std::map<size_t, std::string>&& style_tags)
	: text(std::move(text))
	, style_tags(std::move(style_tags))
{
}

StflRichText operator+(StflRichText left, const StflRichText& right)
{
	auto text = left.text + right.text;
	auto tags = left.style_tags;

	for (const auto& tag_position : right.style_tags) {
		tags[tag_position.first + left.text.size()] = tag_position.second;
	}

	return StflRichText(std::move(text), std::move(tags));
}

StflRichText StflRichText::from_plaintext_string(std::string text)
{
	text = utils::quote_for_stfl(text);

	return from_quoted(text);
}

StflRichText StflRichText::from_quoted(std::string text)
{
	auto style_tags = extract_style_tags(text);

	return StflRichText(std::move(text), std::move(style_tags));
}

void StflRichText::apply_style_tag(const std::string& tag, size_t start, size_t end)
{
	merge_style_tag(style_tags, tag, start, end);
}

std::string StflRichText::get_plaintext() const
{
	return text;
}

std::string StflRichText::stfl_quoted_string() const
{
	return insert_style_tags(text, style_tags);
}

std::map<size_t, std::string> StflRichText::extract_style_tags(std::string& str)
{
	std::map<size_t, std::string> tags;

	size_t pos = 0;
	while (pos < str.size()) {
		auto tag_start = str.find_first_of("<>", pos);
		if (tag_start == std::string::npos) {
			break;
		}
		if (str[tag_start] == '>') {
			// Keep unmatched '>' (stfl way of encoding a literal '>')
			pos = tag_start + 1;
			continue;
		}
		auto tag_end = str.find_first_of("<>", tag_start + 1);
		if (tag_end == std::string::npos) {
			break;
		}
		if (str[tag_end] == '<') {
			// First '<' bracket is unmatched, ignoring it
			pos = tag_start + 1;
			continue;
		}
		if (tag_end - tag_start == 1) {
			// Convert "<>" into "<" (stfl way of encoding a literal '<')
			str.erase(tag_end, 1);
			pos = tag_start + 1;
			continue;
		}
		tags[tag_start] = str.substr(tag_start, tag_end - tag_start + 1);
		str.erase(tag_start, tag_end - tag_start + 1);
		pos = tag_start;
	}
	return tags;
}

void StflRichText::merge_style_tag(std::map<size_t, std::string>& tags,
	const std::string& tag, size_t start, size_t end)
{
	if (end <= start) {
		return;
	}

	// Find the latest tag occurring before `end`.
	// It is important that looping executes in ascending order of location.
	std::string latest_tag = "</>";
	for (const auto& location_tag : tags) {
		size_t location = location_tag.first;
		if (location > end) {
			break;
		}
		latest_tag = location_tag.second;
	}
	tags[start] = tag;
	tags[end] = latest_tag;

	// Remove any old tags between the start and end marker
	for (auto it = tags.begin(); it != tags.end(); ) {
		if (it->first > start && it->first < end) {
			it = tags.erase(it);
		} else {
			++it;
		}
	}
}

std::string StflRichText::insert_style_tags(const std::string& text,
	const std::map<size_t, std::string>& style_tags)
{
	auto str = text;
	auto tags = style_tags;

	// Expand "<" into "<>" (reverse of what happened in extract_style_tags()
	size_t pos = 0;
	while (pos < str.size()) {
		auto bracket = str.find_first_of("<", pos);
		if (bracket == std::string::npos) {
			break;
		}
		pos = bracket + 1;
		// Add to strings in the `tags` map so we don't have to shift all the positions in that map
		// (would be necessary if inserting directly into `str`)
		tags[pos] = ">" + tags[pos];
	}

	for (auto it = tags.rbegin(); it != tags.rend(); ++it) {
		if (it->first > str.length()) {
			// Ignore tags outside of string
			continue;
		}
		str.insert(it->first, it->second);
	}

	return str;
}

}
