#include "textformatter.h"

#include <algorithm>
#include <cinttypes>

#include "htmlrenderer.h"
#include "logger.h"
#include "regexmanager.h"
#include "stflpp.h"
#include "stflrichtext.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

TextFormatter::TextFormatter(
	const std::vector<std::pair<LineType, std::string>>& text)
{
	for (const auto& [type, line] : text) {
		const auto tabs_replaced_line = utils::replace_all(line, "\t", "        ");
		const auto clean_line = utils::wstr2str(
				utils::clean_nonprintable_characters(utils::str2wstr(tabs_replaced_line)));
		lines.push_back(std::make_pair(type, clean_line));
	}
}

bool iswhitespace(const std::string& input)
{
	return std::all_of(input.cbegin(), input.cend(), [](std::string::value_type c) {
		return std::isspace(c);
	});
};

std::vector<std::string> wrap_line(const std::string& line, const size_t width,
	bool raw)
{
	if (line.empty()) {
		return {""};
	}

	std::vector<std::string> result;
	std::vector<std::string> words = utils::tokenize_spaced(line);

	std::string prefix;
	size_t prefix_width = 0;
	auto strwidth = [raw](const std::string& str) {
		if (raw) {
			return utils::strwidth(str);
		} else {
			return utils::strwidth_stfl(str);
		}
	};
	auto substr_with_width = [raw](const std::string& str, const size_t max_width) {
		if (raw) {
			return utils::substr_with_width(str, max_width);
		} else {
			return utils::substr_with_width_stfl(str, max_width);
		}
	};

	if (iswhitespace(words[0])) {
		prefix = substr_with_width(words[0], width);
		prefix_width = strwidth(prefix);
		words.erase(words.cbegin());
	}

	std::string curline = prefix;

	for (auto& word : words) {
		size_t word_width = strwidth(word);
		size_t curline_width = strwidth(curline);

		// for languages (e.g., CJK) don't use a space as a word
		// boundary
		while (word_width > (width - prefix_width)) {
			size_t space_left = width - curline_width;
			std::string part =
				substr_with_width(word, space_left);
			curline.append(part);
			word.erase(0, part.length());
			result.push_back(curline);
			curline = prefix;
			if (part.empty()) {
				// discard the current word
				word.clear();
			}

			word_width = strwidth(word);
			curline_width = strwidth(curline);
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
	std::optional<Dialog> location,
	// wrappable lines are wrapped at this width
	const size_t wrap_width,
	// if non-zero, softwrappable lines are wrapped at this width
	const size_t total_width,
	bool raw = false)
{
	LOG(Level::DEBUG,
		"TextFormatter::format_text_plain: rxman = %p, location = "
		"`%s', "
		"wrap_width = %" PRIu64 ", total_width = %" PRIu64 ", %" PRIu64
		" lines",
		rxman,
		location.has_value() ? dialog_name(location.value()) : "",
		static_cast<uint64_t>(wrap_width),
		static_cast<uint64_t>(total_width),
		static_cast<uint64_t>(lines.size()));

	std::vector<std::string> format_cache;

	auto store_line = [&format_cache](std::string line) {
		format_cache.push_back(line);

		LOG(Level::DEBUG,
			"TextFormatter::format_text_plain: stored `%s'",
			line);
	};

	for (const auto& line : lines) {
		const auto type = line.first;
		auto text = line.second;

		LOG(Level::DEBUG,
			"TextFormatter::format_text_plain: got line `%s' type "
			"%u",
			text,
			static_cast<unsigned int>(type));

		if (rxman && location.has_value() && type != LineType::hr) {
			// TODO: Propagate usage of StflRichText
			auto x = StflRichText::from_quoted(text);
			rxman->quote_and_highlight(x, location.value());
			text = x.stfl_quoted();
		}

		switch (type) {
		case LineType::wrappable:
			if (text.empty() || iswhitespace(text)) {
				store_line(" ");
				continue;
			}
			text = utils::consolidate_whitespace(text);
			for (const auto& line : wrap_line(text, wrap_width, raw)) {
				store_line(line);
			}
			break;

		case LineType::softwrappable:
			if (text.empty() || iswhitespace(text)) {
				store_line(" ");
				continue;
			}
			if (total_width == 0) {
				store_line(text);
			} else {
				for (const auto& line :
					wrap_line(text, total_width, raw)) {
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
	std::optional<Dialog> location,
	const size_t wrap_width,
	const size_t total_width)
{
	auto formatted = format_text_plain_helper(
			lines, rxman, location, wrap_width, total_width);

	auto format_cache = std::string("{list");
	for (auto& line : formatted) {
		if (line != "") {
			utils::trim_end(line);
			format_cache.append(strprintf::fmt(
					"{listitem text:%s}", Stfl::quote(line)));
		}
	}
	format_cache.push_back('}');

	auto line_count = formatted.size();

	return {format_cache, line_count};
}

std::string TextFormatter::format_text_plain(const size_t width,
	const size_t total_width)
{
	std::string result;
	auto formatted = format_text_plain_helper(
			lines, nullptr, {}, width, total_width, true);
	for (const auto& line : formatted) {
		result += line + "\n";
	}

	return result;
}

} // namespace newsboat
