#include <htmlrenderer.h>
#include <tagsouppullparser.h>
#include <utils.h>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <logger.h>
#include <libgen.h>
#include <config.h>

namespace newsbeuter {

htmlrenderer::htmlrenderer(unsigned int width, bool raw) : w(width), raw_(raw) { 
	tags["a"] = TAG_A;
	tags["embed"] = TAG_EMBED;
	tags["br"] = TAG_BR;
	tags["pre"] = TAG_PRE;
	tags["ituneshack"] = TAG_ITUNESHACK;
	tags["img"] = TAG_IMG;
	tags["blockquote"] = TAG_BLOCKQUOTE;
	tags["aside"] = TAG_BLOCKQUOTE;
	tags["p"] = TAG_P;
	tags["h1"] = TAG_H1;
	tags["h2"] = TAG_H2;
	tags["h3"] = TAG_H3;
	tags["h4"] = TAG_H4;
	tags["ol"] = TAG_OL;
	tags["ul"] = TAG_UL;
	tags["li"] = TAG_LI;
	tags["dt"] = TAG_DT;
	tags["dd"] = TAG_DD;
	tags["dl"] = TAG_DL;
	tags["sup"] = TAG_SUP;
	tags["sub"] = TAG_SUB;
	tags["hr"] = TAG_HR;
	tags["b"] = TAG_STRONG;
	tags["strong"] = TAG_STRONG;
	tags["u"] = TAG_UNDERLINE;
	tags["q"] = TAG_QUOTATION;
	tags["script"] = TAG_SCRIPT;
	tags["style"] = TAG_STYLE;
	tags["table"] = TAG_TABLE;
	tags["th"] = TAG_TH;
	tags["tr"] = TAG_TR;
	tags["td"] = TAG_TD;
}

void htmlrenderer::render(const std::string& source, std::vector<std::string>& lines, std::vector<linkpair>& links, const std::string& url) {
	std::istringstream input(source);
	render(input, lines, links, url);
}


unsigned int htmlrenderer::add_link(std::vector<linkpair>& links, const std::string& link, link_type type) {
	bool found = false;
	unsigned int i=1;
	for (std::vector<linkpair>::iterator it=links.begin();it!=links.end();++it, ++i) {
		if (it->first == link) {
			found = true;
			break;
		}
	}
	if (!found)
		links.push_back(linkpair(link,type));

	return i;
}

void htmlrenderer::render(std::istream& input, std::vector<std::string>& lines, std::vector<linkpair>& links, const std::string& url) {
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
	xpp.setInput(input);
	
	for (tagsouppullparser::event e = xpp.next(); e != tagsouppullparser::END_DOCUMENT; e = xpp.next()) {	
		std::string tagname;
		switch (e) {
			case tagsouppullparser::START_TAG:
				tagname = xpp.getText();
				std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
				current_tag = tags[tagname];

				switch (current_tag) {
					case TAG_A: {
							std::string link;
							try {
								link = xpp.getAttributeValue("href");
							} catch (const std::invalid_argument& ) {
								LOG(LOG_WARN,"htmlrenderer::render: found a tag with no href attribute");
								link = "";
							}
							if (link.length() > 0) {
								link_num = add_link(links,utils::censor_url(utils::absolute_url(url,link)), LINK_HREF);
								if (!raw_) 
									curline.append("<u>");
							}
						}
						break;
					case TAG_STRONG:
						if (!raw_)
							curline.append("<b>");
						break;
					case TAG_UNDERLINE:
						if (!raw_)
							curline.append("<u>");
						break;
					case TAG_QUOTATION:
						if (!raw_)
							curline.append("\"");
						break;

					case TAG_EMBED: {
							std::string type;
							try {
								type = xpp.getAttributeValue("type");
							} catch (const std::invalid_argument& ) {
								LOG(LOG_WARN, "htmlrenderer::render: found embed object without type attribute");
								type = "";
							}
							if (type == "application/x-shockwave-flash") {
								std::string link;
								try {
									link = xpp.getAttributeValue("src");
								} catch (const std::invalid_argument& ) {
									LOG(LOG_WARN, "htmlrenderer::render: found embed object without src attribute");
									link = "";
								}
								if (link.length() > 0) {
									unsigned int link_num = add_link(links,utils::censor_url(utils::absolute_url(url,link)), LINK_EMBED);
									curline.append(utils::strprintf("[%s %u]", _("embedded flash:"), link_num));
								}
							}
						}
						break;

					case TAG_BR:
						add_line(curline, tables, lines);
						prepare_newline(curline, tables.size() ? 0 : indent_level);	
						break;

					case TAG_PRE:
						inside_pre = true;
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_ITUNESHACK:
						itunes_hack = true;
						break;

					case TAG_IMG: {
							std::string imgurl;
							try {
								imgurl = xpp.getAttributeValue("src");
							} catch (const std::invalid_argument& ) {
								LOG(LOG_WARN,"htmlrenderer::render: found img tag with no src attribute");
								imgurl = "";
							}
							if (imgurl.length() > 0) {
								unsigned int link_num = add_link(links,utils::censor_url(utils::absolute_url(url,imgurl)), LINK_IMG);
								curline.append(utils::strprintf("[%s %u]", _("image"), link_num));
								image_count++;
							}
						}
						break;

					case TAG_BLOCKQUOTE:
						++indent_level;
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline, tables.size() ? 0 : indent_level);	
						break;

					case TAG_H1:
					case TAG_H2:
					case TAG_H3:
					case TAG_H4:
					case TAG_P:
						add_nonempty_line(curline, tables, lines);
						if (lines.size() > 0 && lines[lines.size()-1].length() > static_cast<unsigned int>(indent_level*2))
							add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_OL:
						is_ol = true;
						{
							unsigned int ol_count = 1;
							try {
								std::string ol_count_str = xpp.getAttributeValue("start");
								std::istringstream is(ol_count_str);
								is >> ol_count;
							} catch (const std::invalid_argument& ) {
								ol_count = 1;
							}
							ol_counts.push_back(ol_count);

							std::string ol_type;
							try {
								ol_type = xpp.getAttributeValue("type");
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
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_UL:
						is_ol = false;
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_LI:
						if (inside_li) {
							indent_level-=2;
							if (indent_level < 0) indent_level = 0;
							add_nonempty_line(curline, tables, lines);
							prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						}
						inside_li = true;
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						indent_level+=2;
						if (is_ol && ol_counts.size() != 0) {
							curline.append(utils::strprintf("%s.", format_ol_count(ol_counts[ol_counts.size()-1], ol_types[ol_types.size()-1]).c_str()));
							++ol_counts[ol_counts.size()-1];
						} else {
							curline.append("  * ");
						}
						break;

					case TAG_DT:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_DD:
						indent_level+=4;
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_DL:
						// ignore tag
						break;

					case TAG_SUP:
						curline.append("^");
						break;

					case TAG_SUB:
						curline.append("[");
						break;

					case TAG_HR:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						add_line(std::string(" ") + std::string(w - 2, '-') + std::string(" "), tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_SCRIPT:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);

						// don't render scripts, ignore current line
						inside_script++;
						break;

					case TAG_STYLE:
						inside_style++;
						break;

					case TAG_TABLE: {
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline, 0); // no indent in tables

						bool border = false;
						try {
							std::string b = xpp.getAttributeValue("border");
							border = (utils::to_u(b) > 0);
						} catch (const std::invalid_argument& ) {
							// is ok, no border than
						}
						tables.push_back(Table(border));
						break;
					}

					case TAG_TR:
						if (!tables.empty())
							tables.back().start_row();
						break;

					case TAG_TH: {
						size_t span = 1;
						try {
							span = utils::to_u(xpp.getAttributeValue("colspan"));
						} catch (const std::invalid_argument& ) {
							// is ok, span 1 than
						}
						if (!tables.empty())
							tables.back().start_cell(span);
							curline.append("<b>");
						break;
					}

					case TAG_TD: {
						size_t span = 1;
						try {
							span = utils::to_u(xpp.getAttributeValue("colspan"));
						} catch (const std::invalid_argument& ) {
							// is ok, span 1 than
						}
						if (!tables.empty())
							tables.back().start_cell(span);
						break;
					}

					default:
						break;
				}
				break;

			case tagsouppullparser::END_TAG:
				tagname = xpp.getText();
				std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
				current_tag = tags[tagname];

				switch (current_tag) {
					case TAG_BLOCKQUOTE:
						--indent_level;
						if (indent_level < 0) indent_level = 0;
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_OL:
						ol_types.pop_back();
						ol_counts.pop_back();
						// fall-through
					case TAG_UL:
						if (inside_li) {
							indent_level-=2;
							if (indent_level < 0) indent_level = 0;
							add_nonempty_line(curline, tables, lines);
							prepare_newline(curline,  tables.size() ? 0 : indent_level);
						}
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_DT:
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_DD:
						indent_level-=4;
						if (indent_level < 0) indent_level = 0;
						add_nonempty_line(curline, tables, lines);
						add_line("", tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);	
						break;

					case TAG_DL:
						// ignore tag
						break;

					case TAG_LI:
						indent_level-=2;
						if (indent_level < 0) indent_level = 0;
						inside_li = false;
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_H1:
						if (line_is_nonempty(curline)) {
							add_line(curline, tables, lines);
							size_t llen = utils::strwidth_stfl(curline);
							prepare_newline(curline,  tables.size() ? 0 : indent_level);
							add_line(std::string(llen, '-'), tables, lines);
						}
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_H2:
					case TAG_H3:
					case TAG_H4:
					case TAG_P:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_PRE:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						inside_pre = false;
						break;

					case TAG_SUB:
						curline.append("]");
						break;
					
					case TAG_SUP:
						// has closing tag, but we render nothing.
						break;

					case TAG_A:
						if (link_num != -1) {
							if (!raw_)
								curline.append("</>");
							curline.append(utils::strprintf("[%d]", link_num));
							link_num = -1;
						}
						break;

					case TAG_UNDERLINE:
						if (!raw_)
							curline.append("</>");
						break;

					case TAG_STRONG:
						if (!raw_)
							curline.append("</>");
						break;

					case TAG_QUOTATION:
						if (!raw_)
							curline.append("\"");
						break;

					case TAG_EMBED:
					case TAG_BR:
					case TAG_ITUNESHACK:
					case TAG_IMG:
					case TAG_HR:
						// ignore closing tags
						break;

					case TAG_SCRIPT:
						// don't render scripts, ignore current line
						if (inside_script)
							inside_script--;
						prepare_newline(curline,  tables.size() ? 0 : indent_level);
						break;

					case TAG_STYLE:
						if (inside_style)
							inside_style--;
						break;

					case TAG_TABLE:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline, 0); // no indent in tables

						if (!tables.empty()) {
							std::vector<std::string> table_text;
							tables.back().complete_cell();
							tables.back().complete_row();
							render_table(tables.back(), table_text);
							tables.pop_back();

							if (!tables.empty()) { // still a table on the outside?
								for(size_t idx=0; idx < table_text.size(); ++idx)
								tables.back().add_text(table_text[idx]); // add rendered table to current cell
							} else {
								for(size_t idx=0; idx < table_text.size(); ++idx) {
									std::string s = table_text[idx];
									while (s.length() > 0 && s[0] == '\n')
										s.erase(0, 1);
									add_line(s, tables, lines);
								}
							}
						}
						prepare_newline(curline, tables.size() ? 0: indent_level);
						break;


					case TAG_TR:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline, 0); // no indent in tables

						if (!tables.empty())
							tables.back().complete_row();
						break;

					case TAG_TH:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline, 0); // no indent in tables

						if (!tables.empty())
							curline.append("</>");
							tables.back().complete_cell();
						break;

					case TAG_TD:
						add_nonempty_line(curline, tables, lines);
						prepare_newline(curline, 0); // no indent in tables

						if (!tables.empty())
							tables.back().complete_cell();
						break;

					default:
						break;
				}
				break;

			case tagsouppullparser::TEXT:
				{
					if (itunes_hack) {
						std::vector<std::string> words = utils::tokenize_nl(utils::quote_for_stfl(xpp.getText()));
						for (std::vector<std::string>::iterator it=words.begin();it!=words.end();++it) {
							if (*it == "\n") {
								add_line(curline, tables, lines);
								prepare_newline(curline,  tables.size() ? 0 : indent_level);
							} else {
								std::vector<std::string> words2 = utils::tokenize_spaced(*it);
								unsigned int i=0;
								bool new_line = false;
								for (std::vector<std::string>::iterator it2=words2.begin();it2!=words2.end();++it2,++i) {
									if ((utils::strwidth_stfl(curline) + utils::strwidth_stfl(*it2)) >= w) {
										add_nonempty_line(curline, tables, lines);
										prepare_newline(curline,  tables.size() ? 0 : indent_level);
										new_line = true;
									}
									if (new_line) {
										if (*it2 != " ")
											curline.append(*it2);
										new_line = false;
									} else {
										curline.append(*it2);
									}
								}
							}
						}
					} else if (inside_pre) {
						std::vector<std::string> words = utils::tokenize_nl(utils::quote_for_stfl(xpp.getText()));
						for (std::vector<std::string>::iterator it=words.begin();it!=words.end();++it) {
							if (*it == "\n") {
								add_line(curline, tables, lines);
								prepare_newline(curline,  tables.size() ? 0 : indent_level);
							} else {
								curline.append(*it);
							}
						}
					} else if (inside_script || inside_style) {
						// skip scripts and CSS styles
					} else {
						std::string s = utils::quote_for_stfl(xpp.getText());
						while (s.length() > 0 && s[0] == '\n')
							s.erase(0, 1);
						std::vector<std::string> words = utils::tokenize_spaced(s);

						unsigned int i=0;
						bool new_line = false;

						if (!line_is_nonempty(curline) && !words.empty() && words[0] == " ") {
							words.erase(words.begin());
						}

						for (std::vector<std::string>::iterator it=words.begin();it!=words.end();++it,++i) {
							if ((utils::strwidth_stfl(curline) + utils::strwidth_stfl(*it)) >= w) {
								add_nonempty_line(curline, tables, lines);
								prepare_newline(curline, tables.size() ? 0 : indent_level);
								new_line = true;
							}
							if (new_line) {
								if (*it != " ")
									curline.append(*it);
								new_line = false;
							} else {
								curline.append(*it);
							}
						}
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
	while(!tables.empty()) {
		std::vector<std::string> table_text;
		render_table(tables.back(), table_text);
		tables.pop_back();
		for(size_t idx=0; idx < table_text.size(); ++idx) {
			std::string s = table_text[idx];
			while (s.length() > 0 && s[0] == '\n')
				s.erase(0, 1);
			add_line(s, tables, lines);
		}
	}

	// add link list
	if (links.size() > 0) {
		lines.push_back("");
		lines.push_back(_("Links: "));
		for (unsigned int i=0;i<links.size();++i) {
			lines.push_back(utils::strprintf("[%u]: %s (%s)", i+1, links[i].first.c_str(), type2str(links[i].second).c_str()));
		}
	}
}

std::string htmlrenderer::type2str(link_type type) {
	switch (type) {
		case LINK_HREF: return _("link");
		case LINK_IMG: return _("image");
		case LINK_EMBED: return _("embedded flash");
		default: return _("unknown (bug)");
	}
}

void htmlrenderer::add_nonempty_line(const std::string& curline, std::vector<Table>& tables, std::vector<std::string>& lines)
{
	if (line_is_nonempty(curline))
		add_line(curline, tables, lines);
}

void htmlrenderer::add_line(const std::string& curline, std::vector<Table>& tables, std::vector<std::string>& lines)
{
	if (tables.size())
		tables.back().add_text(curline);
	else
		lines.push_back(curline);
}

void htmlrenderer::prepare_newline(std::string& line, int indent_level) {
	line = "";
	line.append(indent_level*2, ' ');
}

bool htmlrenderer::line_is_nonempty(const std::string& line) {
	for (unsigned int i=0;i<line.length();++i) {
		if (!isblank(line[i]) && line[i] != '\n' && line[i] && '\r')
			return true;
	}
	return false;
}


void htmlrenderer::TableRow::start_cell(size_t span)
{
	inside = true;
	if (span < 1)
		span = 1;
	cells.push_back(TableCell(span));
}

void htmlrenderer::TableRow::add_text(const std::string& str)
{
	if (!inside)
		start_cell(1); // colspan 1

	cells.back().text.push_back(str);
}

void htmlrenderer::TableRow::complete_cell()
{
	inside = false;
}



void htmlrenderer::Table::start_cell(size_t span)
{
	if (!inside)
		start_row();
	rows.back().start_cell(span);
}

void htmlrenderer::Table::complete_cell()
{
    if (rows.size()) {
	    rows.back().complete_cell();
    }
}

void htmlrenderer::Table::start_row()
{
	if (rows.size() && rows.back().inside)
		rows.back().complete_cell();
	inside = true;
	rows.push_back(TableRow());
}

void htmlrenderer::Table::add_text(const std::string& str)
{
	if (!inside)
		start_row();
	rows.back().add_text(str);
}

void htmlrenderer::Table::complete_row()
{
	inside = false;
}

void htmlrenderer::render_table(const Table& table, std::vector<std::string>& lines)
{
	// get number of rows
	size_t rows = table.rows.size();

	// get maximum number of cells
	size_t cells = 0;
	for(size_t row=0; row < rows; row++) {
		size_t count = 0;
		for(size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
			count += table.rows[row].cells[cell].span;
		}
		cells  = std::max(cells, count);
	}

	// get width of each row
	std::vector<size_t> cell_widths;
	cell_widths.resize(cells, 0);
	for(size_t row=0; row < rows; row++) {
		for(size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
			size_t w = 0;
			if (table.rows[row].cells[cell].text.size()) {
				for(size_t idx=0; idx < table.rows[row].cells[cell].text.size(); idx++)
					w = std::max(w, utils::strwidth_stfl(table.rows[row].cells[cell].text[idx]));
			}
			if (table.rows[row].cells[cell].span > 1) {
				w += table.rows[row].cells[cell].span;
				w /= table.rows[row].cells[cell].span; // devide size evenly on columns (can be done better, I know)
			}
			cell_widths[cell] = std::max(cell_widths[cell], w);
		}
	}

	char hsep = '-';
	char vsep = '|';
	char hvsep = '+';

	// create a row separator
	std::string separator;
	if (table.border)
		separator += hvsep;
	for(size_t cell=0; cell < cells; cell++) {
		separator += std::string(cell_widths[cell], hsep);
		separator += hvsep;
	}

	if (!table.border)
		vsep = ' ';

	// render the table
	if (table.border)
		lines.push_back(separator);
	for(size_t row=0; row < rows; row++) {
		// calc height of this row
		size_t height = 0;
		for(size_t cell=0; cell < table.rows[row].cells.size(); cell++)
			height = std::max(height, table.rows[row].cells[cell].text.size());

		for(size_t idx=0; idx < height; ++idx) {
			std::string line;
			if (table.border)
				line += vsep;
			for(size_t cell=0; cell < table.rows[row].cells.size(); cell++) {
				size_t cell_width = 0;
				if (idx < table.rows[row].cells[cell].text.size()) {
					cell_width = utils::strwidth_stfl(table.rows[row].cells[cell].text[idx]);
					line += table.rows[row].cells[cell].text[idx];
				}
				size_t reference_width = cell_widths[cell];
				if (table.rows[row].cells[cell].span > 1) {
					for(size_t ic=cell+1; ic < cell + table.rows[row].cells[cell].span; ++ic)
						reference_width += cell_widths[ic]+1;
				}
				if (cell_width < reference_width) // pad, if necessary
					line += std::string(reference_width - cell_width, ' ');

				if (cell < table.rows[row].cells.size()-1)
					line += vsep;
			}
			if (table.border)
				line += vsep;
			lines.push_back(line);
		}
		if (table.border)
			lines.push_back(separator);
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
	for (unsigned int i=0;i<(sizeof(values)/sizeof(values[0]));i++) {
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
			return utils::strprintf("%2u", count);
	}
}

}

