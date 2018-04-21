#include "htmlrenderer.h"

#include <sstream>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <libgen.h>

#include "tagsouppullparser.h"
#include "utils.h"
#include "strprintf.h"
#include "logger.h"
#include "config.h"

namespace newsboat {

htmlrenderer::htmlrenderer(bool raw) : raw_(raw) {
	tags["a"] = htmltag::A;
	tags["embed"] = htmltag::EMBED;
	tags["br"] = htmltag::BR;
	tags["pre"] = htmltag::PRE;
	tags["ituneshack"] = htmltag::ITUNESHACK;
	tags["img"] = htmltag::IMG;
	tags["blockquote"] = htmltag::BLOCKQUOTE;
	tags["aside"] = htmltag::BLOCKQUOTE;
	tags["p"] = htmltag::P;
	tags["h1"] = htmltag::H1;
	tags["h2"] = htmltag::H2;
	tags["h3"] = htmltag::H3;
	tags["h4"] = htmltag::H4;
	tags["h5"] = htmltag::H5;
	tags["h6"] = htmltag::H6;
	tags["ol"] = htmltag::OL;
	tags["ul"] = htmltag::UL;
	tags["li"] = htmltag::LI;
	tags["dt"] = htmltag::DT;
	tags["dd"] = htmltag::DD;
	tags["dl"] = htmltag::DL;
	tags["sup"] = htmltag::SUP;
	tags["sub"] = htmltag::SUB;
	tags["hr"] = htmltag::HR;
	tags["b"] = htmltag::STRONG;
	tags["strong"] = htmltag::STRONG;
	tags["u"] = htmltag::UNDERLINE;
	tags["q"] = htmltag::QUOTATION;
	tags["script"] = htmltag::SCRIPT;
	tags["style"] = htmltag::STYLE;
	tags["table"] = htmltag::TABLE;
	tags["th"] = htmltag::TH;
	tags["tr"] = htmltag::TR;
	tags["td"] = htmltag::TD;
}

void htmlrenderer::render(
		const std::string& source,
		std::vector<std::pair<LineType, std::string>>& lines,
		std::vector<linkpair>& links,
		const std::string& url)
{
	std::istringstream input(source);
	render(input, lines, links, url);
}


unsigned int htmlrenderer::add_link(
		std::vector<linkpair>& links,
		const std::string& link, link_type type)
{
	bool found = false;
	unsigned int i=1;
	for (auto l : links) {
		if (l.first == link) {
			found = true;
			break;
		}
		i++;
	}
	if (!found)
		links.push_back(linkpair(link,type));

	return i;
}

void htmlrenderer::render(
		std::istream& input,
		std::vector<std::pair<LineType, std::string>>& lines,
		std::vector<linkpair>& links,
		const std::string& url)
{
	unsigned int image_count = 0;
	std::string curline;
	int indent_level = 0;
	bool inside_li = false, is_ol = false, inside_pre = false;
	bool itunes_hack = false;
	size_t inside_script = 0;
	size_t inside_style = 0;
	std::vector<unsigned int> ol_counts;
	std::vector<char> ol_types;
	htmltag current_tag;
	int link_num = -1;
	std::vector<Table> tables;

	/*
	 * to render the HTML, we use a self-developed "XML" pull parser.
	 *
	 * A pull parser works like this:
	 *   - we feed it with an XML stream
	 *   - we then gather an iterator
	 *   - we then can iterate over all continuous elements, such as start tag, close tag, text element, ...
	 */
	tagsouppullparser xpp;
	xpp.set_input(input);

	for (tagsouppullparser::event e = xpp.next(); e != tagsouppullparser::event::END_DOCUMENT; e = xpp.next()) {
		std::string tagname;
		switch (e) {
		case tagsouppullparser::event::START_TAG:
			tagname = xpp.get_text();
			std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
			current_tag = tags[tagname];

			switch (current_tag) {
			case htmltag::A: {
				std::string link;
				try {
					link = xpp.get_attribute_value("href");
				} catch (const std::invalid_argument& ) {
					LOG(level::WARN,"htmlrenderer::render: found a tag with no href attribute");
					link = "";
				}
				if (link.length() > 0) {
					link_num = add_link(links,utils::censor_url(utils::absolute_url(url,link)), link_type::HREF);
					if (!raw_)
						curline.append("<u>");
				}
			}
			break;
			case htmltag::STRONG:
				if (!raw_)
					curline.append("<b>");
				break;
			case htmltag::UNDERLINE:
				if (!raw_)
					curline.append("<u>");
				break;
			case htmltag::QUOTATION:
				if (!raw_)
					curline.append("\"");
				break;

			case htmltag::EMBED: {
				std::string type;
				try {
					type = xpp.get_attribute_value("type");
				} catch (const std::invalid_argument& ) {
					LOG(level::WARN, "htmlrenderer::render: found embed object without type attribute");
					type = "";
				}
				if (type == "application/x-shockwave-flash") {
					std::string link;
					try {
						link = xpp.get_attribute_value("src");
					} catch (const std::invalid_argument& ) {
						LOG(level::WARN, "htmlrenderer::render: found embed object without src attribute");
						link = "";
					}
					if (link.length() > 0) {
						link_num = add_link(links,utils::censor_url(utils::absolute_url(url,link)), link_type::EMBED);
						curline.append(strprintf::fmt("[%s %u]", _("embedded flash:"), link_num));
					}
				}
			}
			break;

			case htmltag::BR:
				add_line(curline, tables, lines);
				prepare_new_line(curline, tables.size() ? 0 : indent_level);
				break;

			case htmltag::PRE:
				inside_pre = true;
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::ITUNESHACK:
				itunes_hack = true;
				break;

			case htmltag::IMG: {
				std::string imgurl;
				std::string imgtitle;
				try {
					imgurl = xpp.get_attribute_value("src");
				} catch (const std::invalid_argument& ) {
					LOG(level::WARN,"htmlrenderer::render: found img tag with no src attribute");
					imgurl = "";
				}
				try {
					imgtitle = xpp.get_attribute_value("title");
				} catch (const std::invalid_argument& ) {
					imgtitle = "";
				}
				if (imgurl.length() > 0) {
					if (imgurl.substr(0,5) == "data:") {
						link_num = add_link(links, "inline image", link_type::IMG);
					} else {
						link_num = add_link(links,utils::censor_url(utils::absolute_url(url,imgurl)), link_type::IMG);
					}
					if (imgtitle != "") {
						curline.append(strprintf::fmt("[%s %u: %s]", _("image"), link_num, imgtitle));
					} else {
						curline.append(strprintf::fmt("[%s %u]", _("image"), link_num));
					}
					image_count++;
				}
			}
			break;

			case htmltag::BLOCKQUOTE:
				++indent_level;
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline, tables.size() ? 0 : indent_level);
				break;

			case htmltag::H1:
			case htmltag::H2:
			case htmltag::H3:
			case htmltag::H4:
			case htmltag::H5:
			case htmltag::H6:
			case htmltag::P:
				{
				add_nonempty_line(curline, tables, lines);
				if (lines.size() > 0) {
					std::string::size_type last_line_len =
						lines[lines.size()-1].second.length();
					if (last_line_len > static_cast<unsigned int>(indent_level*2))
						add_line("", tables, lines);
				}
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				}
				break;

			case htmltag::OL:
				is_ol = true;
				{
					unsigned int ol_count = 1;
					std::string ol_count_str;
					try {
						ol_count_str = xpp.get_attribute_value("start");
					} catch (const std::invalid_argument& ) {
						ol_count_str = "1";
					}
					ol_count = utils::to_u(ol_count_str, 1);
					ol_counts.push_back(ol_count);

					std::string ol_type;
					try {
						ol_type = xpp.get_attribute_value("type");
						if (ol_type != "1" && ol_type != "a" && ol_type != "A" && ol_type != "i" && ol_type != "I") {
							ol_type = "1";
						}
					} catch (const std::invalid_argument& ) {
						ol_type = "1";
					}
					ol_types.push_back(ol_type[0]);
				}
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::UL:
				is_ol = false;
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::LI:
				if (inside_li) {
					indent_level-=2;
					if (indent_level < 0) indent_level = 0;
					add_nonempty_line(curline, tables, lines);
					prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				}
				inside_li = true;
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				indent_level+=2;
				if (is_ol && ol_counts.size() != 0) {
					curline.append(strprintf::fmt("%s. ", format_ol_count(ol_counts[ol_counts.size()-1], ol_types[ol_types.size()-1])));
					++ol_counts[ol_counts.size()-1];
				} else {
					curline.append("  * ");
				}
				break;

			case htmltag::DT:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::DD:
				indent_level+=4;
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::DL:
				// ignore tag
				break;

			case htmltag::SUP:
				curline.append("^");
				break;

			case htmltag::SUB:
				curline.append("[");
				break;

			case htmltag::HR:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				add_hr(lines);
				break;

			case htmltag::SCRIPT:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);

				// don't render scripts, ignore current line
				inside_script++;
				break;

			case htmltag::STYLE:
				inside_style++;
				break;

			case htmltag::TABLE: {
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline, 0); // no indent in tables

				bool has_border = false;
				try {
					std::string b = xpp.get_attribute_value("border");
					has_border = (utils::to_u(b, 0) > 0);
				} catch (const std::invalid_argument& ) {
					// is ok, no border then
				}
				tables.push_back(Table(has_border));
				break;
			}

			case htmltag::TR:
				if (!tables.empty())
					tables.back().start_row();
				break;

			case htmltag::TH: {
				size_t span = 1;
				try {
					span = utils::to_u(xpp.get_attribute_value("colspan"), 1);
				} catch (const std::invalid_argument& ) {
					// is ok, span 1 then
				}
				if (!tables.empty())
					tables.back().start_cell(span);
				curline.append("<b>");
				break;
			}

			case htmltag::TD: {
				size_t span = 1;
				try {
					span = utils::to_u(xpp.get_attribute_value("colspan"), 1);
				} catch (const std::invalid_argument& ) {
					// is ok, span 1 then
				}
				if (!tables.empty())
					tables.back().start_cell(span);
				break;
			}
			}
			break;

		case tagsouppullparser::event::END_TAG:
			tagname = xpp.get_text();
			std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
			current_tag = tags[tagname];

			switch (current_tag) {
			case htmltag::BLOCKQUOTE:
				--indent_level;
				if (indent_level < 0) indent_level = 0;
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::OL:
				ol_types.pop_back();
				ol_counts.pop_back();
			// fall-through
			case htmltag::UL:
				if (inside_li) {
					indent_level-=2;
					if (indent_level < 0) indent_level = 0;
					add_nonempty_line(curline, tables, lines);
					prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				}
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::DT:
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::DD:
				indent_level-=4;
				if (indent_level < 0) indent_level = 0;
				add_nonempty_line(curline, tables, lines);
				add_line("", tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::DL:
				// ignore tag
				break;

			case htmltag::LI:
				indent_level-=2;
				if (indent_level < 0) indent_level = 0;
				inside_li = false;
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::H1:
				if (line_is_nonempty(curline)) {
					add_line(curline, tables, lines);
					size_t llen = utils::strwidth_stfl(curline);
					prepare_new_line(curline,  tables.size() ? 0 : indent_level);
					add_line(std::string(llen, '-'), tables, lines);
				}
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::H2:
			case htmltag::H3:
			case htmltag::H4:
			case htmltag::H5:
			case htmltag::H6:
			case htmltag::P:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::PRE:
				add_line_softwrappable(curline, lines);
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				inside_pre = false;
				break;

			case htmltag::SUB:
				curline.append("]");
				break;

			case htmltag::SUP:
				// has closing tag, but we render nothing.
				break;

			case htmltag::A:
				if (link_num != -1) {
					if (!raw_)
						curline.append("</>");
					curline.append(strprintf::fmt("[%d]", link_num));
					link_num = -1;
				}
				break;

			case htmltag::UNDERLINE:
				if (!raw_)
					curline.append("</>");
				break;

			case htmltag::STRONG:
				if (!raw_)
					curline.append("</>");
				break;

			case htmltag::QUOTATION:
				if (!raw_)
					curline.append("\"");
				break;

			case htmltag::EMBED:
			case htmltag::BR:
			case htmltag::ITUNESHACK:
			case htmltag::IMG:
			case htmltag::HR:
				// ignore closing tags
				break;

			case htmltag::SCRIPT:
				// don't render scripts, ignore current line
				if (inside_script)
					inside_script--;
				prepare_new_line(curline,  tables.size() ? 0 : indent_level);
				break;

			case htmltag::STYLE:
				if (inside_style)
					inside_style--;
				break;

			case htmltag::TABLE:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline, 0); // no indent in tables

				if (!tables.empty()) {
					std::vector<std::pair<LineType, std::string>> table_text;
					tables.back().complete_cell();
					tables.back().complete_row();
					render_table(tables.back(), table_text);
					tables.pop_back();

					if (!tables.empty()) { // still a table on the outside?
						for (size_t idx=0; idx < table_text.size(); ++idx)
							tables.back().add_text(table_text[idx].second); // add rendered table to current cell
					} else {
						for (size_t idx=0; idx < table_text.size(); ++idx) {
							std::string s = table_text[idx].second;
							while (s.length() > 0 && s[0] == '\n')
								s.erase(0, 1);
							add_line_nonwrappable(s, lines);
						}
					}
				}
				prepare_new_line(curline, tables.size() ? 0: indent_level);
				break;


			case htmltag::TR:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline, 0); // no indent in tables

				if (!tables.empty())
					tables.back().complete_row();
				break;

			case htmltag::TH:
				if (!tables.empty()) {
					curline.append("</>");
				}

				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline, 0); // no indent in tables

				if (!tables.empty()) {
					tables.back().complete_cell();
				}
				break;

			case htmltag::TD:
				add_nonempty_line(curline, tables, lines);
				prepare_new_line(curline, 0); // no indent in tables

				if (!tables.empty())
					tables.back().complete_cell();
				break;
			}
			break;

		case tagsouppullparser::event::TEXT: {
			auto text = utils::quote_for_stfl(xpp.get_text());
			if (itunes_hack) {
				std::vector<std::string> paragraphs = utils::tokenize_nl(text);
				for (auto paragraph : paragraphs) {
					if (paragraph != "\n") {
						add_nonempty_line(curline, tables, lines);
						prepare_new_line(curline,  tables.size() ? 0 : indent_level);
						curline.append(paragraph);
					}
				}
			} else if (inside_pre) {
				std::vector<std::string> paragraphs = utils::tokenize_nl(text);
				for (auto paragraph : paragraphs) {
					if (paragraph == "\n") {
						add_line_softwrappable(curline, lines);
						prepare_new_line(curline,  tables.size() ? 0 : indent_level);
					} else {
						curline.append(paragraph);
					}
				}
			} else if (inside_script || inside_style) {
				// skip scripts and CSS styles
			} else {
				//strip leading whitespace
				bool had_whitespace = false;
				while (text.length() > 0 && (text[0] == '\n' || text[0] == ' ')) {
					text.erase(0, 1);
					had_whitespace = true;
				}
				if (line_is_nonempty(curline) && had_whitespace) {
					curline.append(" ");
				}
				//strip newlines
				text = utils::replace_all(text, "\n", " ");
				curline.append(text);
			}
		}
		break;
		default:
			/* do nothing */
			break;
		}
	}

	// and the rest
	add_nonempty_line(curline, tables, lines);

	// force all tables to be closed and rendered
	while (!tables.empty()) {
		std::vector<std::pair<LineType, std::string>> table_text;
		render_table(tables.back(), table_text);
		tables.pop_back();
		for (size_t idx=0; idx < table_text.size(); ++idx) {
			std::string s = table_text[idx].second;
			while (s.length() > 0 && s[0] == '\n')
				s.erase(0, 1);
			add_line_nonwrappable(s, lines);
		}
	}

	// add link list
	if (links.size() > 0) {
		add_line("", tables, lines);
		add_line(_("Links: "), tables, lines);
		for (unsigned int i=0; i<links.size(); ++i) {
			auto link_text = strprintf::fmt(
					"[%u]: %s (%s)",
					i+1,
					links[i].first,
					type2str(links[i].second));
			add_line_softwrappable(link_text, lines);
		}
	}
}

std::string htmlrenderer::render_hr(const unsigned int width) {
	std::string result = "\n ";
	result += std::string(width - 2, '-');
	result += " \n";

	return result;
}

std::string htmlrenderer::type2str(link_type type) {
	switch (type) {
	case link_type::HREF:
		return _("link");
	case link_type::IMG:
		return _("image");
	case link_type::EMBED:
		return _("embedded flash");
	default:
		return _("unknown (bug)");
	}
}

void htmlrenderer::add_nonempty_line(
		const std::string& curline,
		std::vector<Table>& tables,
		std::vector<std::pair<LineType, std::string>>& lines)
{
	if (line_is_nonempty(curline))
		add_line(curline, tables, lines);
}

void htmlrenderer::add_hr(std::vector<std::pair<LineType, std::string>>& lines) {
	lines.push_back(std::make_pair(LineType::hr, std::string("")));
}

void htmlrenderer::add_line(
		const std::string& curline,
		std::vector<Table>& tables,
		std::vector<std::pair<LineType, std::string>>& lines)
{
	if (tables.size())
		tables.back().add_text(curline);
	else
		lines.push_back(std::make_pair(LineType::wrappable, curline));
}

void htmlrenderer::add_line_softwrappable(
		const std::string& line,
		std::vector<std::pair<LineType, std::string>>& lines)
{
	lines.push_back(std::make_pair(LineType::softwrappable, line));
}

void htmlrenderer::add_line_nonwrappable(
		const std::string& line,
		std::vector<std::pair<LineType, std::string>>& lines)
{
	lines.push_back(std::make_pair(LineType::nonwrappable, line));
}

void htmlrenderer::prepare_new_line(std::string& line, int indent_level) {
	line = "";
	line.append(indent_level*2, ' ');
}

bool htmlrenderer::line_is_nonempty(const std::string& line) {
	for (std::string::size_type i=0; i<line.length(); ++i) {
		if (!isblank(line[i]) && line[i] != '\n' && line[i] != '\r')
			return true;
	}
	return false;
}


void htmlrenderer::TableRow::start_cell(size_t span) {
	inside = true;
	if (span < 1)
		span = 1;
	cells.push_back(TableCell(span));
}

void htmlrenderer::TableRow::add_text(const std::string& str) {
	if (!inside)
		start_cell(1); // colspan 1

	cells.back().text.push_back(str);
}

void htmlrenderer::TableRow::complete_cell() {
	inside = false;
}



void htmlrenderer::Table::start_cell(size_t span) {
	if (!inside)
		start_row();
	rows.back().start_cell(span);
}

void htmlrenderer::Table::complete_cell() {
	if (rows.size()) {
		rows.back().complete_cell();
	}
}

void htmlrenderer::Table::start_row() {
	if (rows.size() && rows.back().inside)
		rows.back().complete_cell();
	inside = true;
	rows.push_back(TableRow());
}

void htmlrenderer::Table::add_text(const std::string& str) {
	if (!inside)
		start_row();
	rows.back().add_text(str);
}

void htmlrenderer::Table::complete_row() {
	inside = false;
}

void htmlrenderer::render_table(
		const htmlrenderer::Table& table,
		std::vector<std::pair<LineType, std::string>>& lines)
{
	// get number of rows
	size_t rows = table.rows.size();

	// get maximum number of cells
	size_t cells = 0;
	for (size_t row=0; row < rows; row++) {
		size_t count = 0;
		for (size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
			count += table.rows[row].cells[cell].span;
		}
		cells  = std::max(cells, count);
	}

	// get width of each row
	std::vector<size_t> cell_widths;
	cell_widths.resize(cells, 0);
	for (size_t row=0; row < rows; row++) {
		for (size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
			size_t width = 0;
			if (table.rows[row].cells[cell].text.size()) {
				for (size_t idx=0; idx < table.rows[row].cells[cell].text.size(); idx++)
					width = std::max(width, utils::strwidth_stfl(table.rows[row].cells[cell].text[idx]));
			}
			if (table.rows[row].cells[cell].span > 1) {
				width += table.rows[row].cells[cell].span;
				width /= table.rows[row].cells[cell].span; // devide size evenly on columns (can be done better, I know)
			}
			cell_widths[cell] = std::max(cell_widths[cell], width);
		}
	}

	char hsep = '-';
	char vsep = '|';
	char hvsep = '+';

	// create a row separator
	std::string separator;
	if (table.has_border)
		separator += hvsep;
	for (size_t cell=0; cell < cells; cell++) {
		separator += std::string(cell_widths[cell], hsep);
		separator += hvsep;
	}

	if (!table.has_border)
		vsep = ' ';

	// render the table
	if (table.has_border)
		lines.push_back(std::make_pair(LineType::nonwrappable, separator));
	for (size_t row=0; row < rows; row++) {
		// calc height of this row
		size_t height = 0;
		for (size_t cell=0; cell < table.rows[row].cells.size(); cell++)
			height = std::max(height, table.rows[row].cells[cell].text.size());

		for (size_t idx=0; idx < height; ++idx) {
			std::string line;
			if (table.has_border)
				line += vsep;
			for (size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
				size_t cell_width = 0;
				if (idx < table.rows[row].cells[cell].text.size()) {
					LOG(level::DEBUG, "row = %d cell = %d text = %s", row, cell, table.rows[row].cells[cell].text[idx]);
					cell_width = utils::strwidth_stfl(table.rows[row].cells[cell].text[idx]);
					line += table.rows[row].cells[cell].text[idx];
				}
				size_t reference_width = cell_widths[cell];
				if (table.rows[row].cells[cell].span > 1) {
					for (size_t ic=cell+1; ic < cell + table.rows[row].cells[cell].span; ++ic)
						reference_width += cell_widths[ic]+1;
				}
				LOG(level::DEBUG, "cell_width = %d reference_width = %d", cell_width, reference_width);
				if (cell_width < reference_width) // pad, if necessary
					line += std::string(reference_width - cell_width, ' ');

				if (cell < table.rows[row].cells.size()-1)
					line += vsep;
			}
			if (table.has_border)
				line += vsep;
			lines.push_back(std::make_pair(LineType::nonwrappable, line));
		}
		if (table.has_border)
			lines.push_back(std::make_pair(LineType::nonwrappable, separator));
	}
}

std::string htmlrenderer::get_char_numbering(unsigned int count) {
	std::string result;
	do {
		count--;
		result.append(1, 'a'+(count % 26));
		count /= 26;
	} while (count > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

std::string htmlrenderer::get_roman_numbering(unsigned int count) {
	unsigned int values[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
	const char * numerals[] = { "m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i" };
	std::string result;
	for (unsigned int i=0; i<(sizeof(values)/sizeof(values[0])); i++) {
		while (count >= values[i]) {
			count -= values[i];
			result.append(numerals[i]);
		}
	}
	return result;
}


std::string htmlrenderer::format_ol_count(unsigned int count, char type) {
	switch (type) {
	case 'a':
		return get_char_numbering(count);
	case 'A': {
		std::string num = get_char_numbering(count);
		std::transform(num.begin(), num.end(), num.begin(), ::toupper);
		return num;
	}
	case 'i':
		return get_roman_numbering(count);
	case 'I': {
		std::string roman = get_roman_numbering(count);
		std::transform(roman.begin(), roman.end(), roman.begin(), ::toupper);
		return roman;
	}
	case '1':
	default:
		return strprintf::fmt("%2u", count);
	}
}

}

