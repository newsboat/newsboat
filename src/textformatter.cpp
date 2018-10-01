#include "textformatter.h"

#include <algorithm>
#include <assert.h>
#include <limits.h>

#include "htmlrenderer.h"
#include "stflpp.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

TextFormatter::TextFormatter() {}

TextFormatter::~TextFormatter() {}

void TextFormatter::add_line(LineType type, std::string line)
{
	LOG(Level::DEBUG,
		"TextFormatter::add_line: `%s' (line type %i)",
		line,
		type);

	auto clean_line = Utils::wstr2str(
		Utils::clean_nonprintable_characters(Utils::str2wstr(line)));
	lines.push_back(std::make_pair(type, clean_line));
}

void TextFormatter::add_lines(
	const std::vector<std::pair<LineType, std::string>>& lines)
{
	for (const auto& line : lines) {
		add_line(line.first,
			Utils::replace_all(line.second, "\t", "        "));
	}
}

std::vector<std::string> wrap_line(const std::string& line, const size_t width)
{
	if (line.empty()) {
		return {""};
	}

	std::vector<std::string> result;
	std::vector<std::string> words = Utils::tokenize_spaced(line);

	std::string prefix;
	size_t prefix_width = 0;
	auto iswhitespace = [](const std::string& input) {
		return std::all_of(input.cbegin(),
			input.cend(),
			[](std::string::value_type c) {
				return std::isspace(c);
			});
	};
	if (iswhitespace(words[0])) {
		prefix = Utils::substr_with_width(words[0], width);
		prefix_width = Utils::strwidth_stfl(prefix);
		words.erase(words.cbegin());
	}

	std::string curline = prefix;

	for (auto& word : words) {
		size_t word_width = Utils::strwidth_stfl(word);
		size_t curline_width = Utils::strwidth_stfl(curline);

		// for languages (e.g., CJK) don't use a space as a word
		// boundary
		while (word_width > (width - prefix_width)) {
			size_t space_left = width - curline_width;
			std::string part =
				Utils::substr_with_width(word, space_left);
			curline.append(part);
			word.erase(0, part.length());
			result.push_back(curline);
			curline = prefix;
			if (part.empty()) {
				// discard the current word
				word.clear();
			}

			word_width = Utils::strwidth_stfl(word);
			curline_width = Utils::strwidth_stfl(curline);
		}

		if ((curline_width + word_width) > width) {
			result.push_back(curline);
			if (iswhitespace(word)) {
				curline = prefix;
			} else {
				curline = prefix + word;
			}
		} else {
			curline.append(word);
		}
	}

	if (curline != prefix) {
		result.push_back(curline);
	}

	return result;
}

std::vector<std::string> format_text_plain_helper(
	const std::vector<std::pair<LineType, std::string>>& lines,
	RegexManager* rxman,
	const std::string& location,
	// wrappable lines are wrapped at this width
	const size_t wrap_width,
	// if non-zero, softwrappable lines are wrapped at this width
	const size_t total_width)
{
	LOG(Level::DEBUG,
		"TextFormatter::format_text_plain: rxman = %p, location = "
		"`%s', "
		"wrap_width = %zu, total_width = %zu, %u lines",
		rxman,
		location,
		wrap_width,
		total_width,
		lines.size());

	std::vector<std::string> format_cache;

	auto store_line = [&format_cache](std::string line) {
		format_cache.push_back(line);

		LOG(Level::DEBUG,
			"TextFormatter::format_text_plain: stored `%s'",
			line);
	};

	for (const auto& line : lines) {
		auto type = line.first;
		auto text = line.second;

		LOG(Level::DEBUG,
			"TextFormatter::format_text_plain: got line `%s' type "
			"%u",
			text,
			type);

		if (rxman && type != LineType::hr) {
			rxman->quote_and_highlight(text, location);
		}

		switch (type) {
		case LineType::wrappable:
			if (text == "") {
				store_line(" ");
				continue;
			}
			text = Utils::consolidate_whitespace(text);
			for (const auto& line : wrap_line(text, wrap_width)) {
				store_line(line);
			}
			break;

		case LineType::softwrappable:
			if (text == "") {
				store_line(" ");
				continue;
			}
			if (total_width == 0) {
				store_line(text);
			} else {
				for (const auto& line :
					wrap_line(text, total_width)) {
					store_line(line);
				}
			}
			break;

		case LineType::nonwrappable:
			store_line(text);
			break;

		case LineType::hr:
			store_line(HtmlRenderer::render_hr(wrap_width));
			break;
		}
	}

	return format_cache;
}

std::pair<std::string, std::size_t> TextFormatter::format_text_to_list(
	RegexManager* rxman,
	const std::string& location,
	const size_t wrap_width,
	const size_t total_width)
{
	auto Formatted = format_text_plain_helper(
		lines, rxman, location, wrap_width, total_width);

	auto format_cache = std::string("{list");
	for (auto& line : Formatted) {
		if (line != "") {
			Utils::trim_end(line);
			format_cache.append(StrPrintf::fmt(
				"{listitem text:%s}", Stfl::quote(line)));
		}
	}
	format_cache.append(1, '}');

	auto line_count = Formatted.size();

	return {format_cache, line_count};
}

std::string TextFormatter::format_text_plain(const size_t width,
	const size_t total_width)
{
	std::string result;
	auto Formatted = format_text_plain_helper(
		lines, nullptr, "", width, total_width);
	for (const auto& line : Formatted) {
		result += line + "\n";
	}

	return result;
}

} // namespace newsboat
