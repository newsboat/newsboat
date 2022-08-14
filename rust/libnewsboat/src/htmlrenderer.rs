use std::collections::HashMap;

use gettextrs::gettext;

use crate::logger::{self, Level};
use crate::tagsouppullparser::{Event, TagSoupPullParser};
use crate::utils;

/// Types of links that HtmlRenderer collects while rendering
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum LinkType {
    /// Ordinary HTML `<a href="...">...</a>` link
    Href,

    /// HTML `<img>`
    Img,

    /// HTML `<embed>`
    Embed,

    /// HTML `<iframe>`
    Iframe,

    /// HTML `<video>`
    Video,

    /// HTML `<audio>`
    Audio,
    // This enum has to be kept in sync with enum LinkType in include/htmlrenderer.h
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub enum HtmlTag {
    Unknown = 0,
    A = 1,
    EMBED,
    IFRAME,
    BR,
    PRE,
    ITUNESHACK,
    IMG,
    BLOCKQUOTE,
    H1,
    H2,
    H3,
    H4,
    H5,
    H6,
    P,
    DIV,
    OL,
    UL,
    LI,
    DT,
    DD,
    DL,
    SUP,
    SUB,
    HR,
    STRONG,
    UNDERLINE,
    QUOTATION,
    SCRIPT,
    STYLE,
    TABLE,
    TH,
    TR,
    TD,
    VIDEO,
    AUDIO,
    SOURCE,
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Debug)]
struct TableCell {
    span: u32,
    text: Vec<String>,
}

impl TableCell {
    fn new(span: u32) -> Self {
        Self {
            span,
            text: Vec::new(),
        }
    }
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Debug)]
struct TableRow {
    inside: bool, // inside a cell
    cells: Vec<TableCell>,
}

impl TableRow {
    fn new() -> Self {
        Self {
            inside: false,
            cells: Vec::new(),
        }
    }

    fn add_text(&mut self, s: &str) {
        if !self.inside {
            self.start_cell(1); // colspan 1
        }
        self.cells
            .last_mut()
            .map(|cell| cell.text.push(s.to_string()));
    }

    fn start_cell(&mut self, span: u32) {
        self.inside = true;
        self.cells.push(TableCell::new(span.max(1)));
    }

    fn complete_cell(&mut self) {
        self.inside = false;
    }
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Debug)]
struct Table {
    inside: bool,
    has_border: bool,
    rows: Vec<TableRow>,
}

impl Table {
    pub fn new(has_border: bool) -> Self {
        Self {
            inside: false,
            has_border,
            rows: Vec::new(),
        }
    }

    fn add_text(&mut self, s: &str) {
        if !self.inside {
            self.start_row();
        }
        self.rows.last_mut().map(|row| row.add_text(s));
    }

    fn start_row(&mut self) {
        self.rows.last_mut().map(|row| {
            if row.inside {
                row.complete_cell();
            }
        });
        self.inside = true;
        self.rows.push(TableRow::new());
    }

    fn complete_row(&mut self) {
        self.inside = false;
    }

    fn start_cell(&mut self, span: u32) {
        if !self.inside {
            self.start_row();
        }
        self.rows.last_mut().map(|row| row.start_cell(span));
    }

    fn complete_cell(&mut self) {
        self.rows.last_mut().map(|row| row.complete_cell());
    }
}

/// This must be kept in sync with include/textformatter.h
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub enum LineType {
    Wrappable = 1,
    Softwrappable,
    Nonwrappable,
    Hr,
}

type LinkPair = (String, LinkType);

pub struct HtmlRenderer {
    tags: HashMap<&'static str, HtmlTag>,
    raw_: bool,
}

impl HtmlRenderer {
    pub fn new(raw: bool) -> Self {
        let tags = HashMap::from([
            ("a", HtmlTag::A),
            ("embed", HtmlTag::EMBED),
            ("iframe", HtmlTag::IFRAME),
            ("br", HtmlTag::BR),
            ("pre", HtmlTag::PRE),
            ("ituneshack", HtmlTag::ITUNESHACK),
            ("img", HtmlTag::IMG),
            ("blockquote", HtmlTag::BLOCKQUOTE),
            ("aside", HtmlTag::BLOCKQUOTE),
            ("p", HtmlTag::P),
            ("div", HtmlTag::DIV),
            ("h1", HtmlTag::H1),
            ("h2", HtmlTag::H2),
            ("h3", HtmlTag::H3),
            ("h4", HtmlTag::H4),
            ("h5", HtmlTag::H5),
            ("h6", HtmlTag::H6),
            ("ol", HtmlTag::OL),
            ("ul", HtmlTag::UL),
            ("li", HtmlTag::LI),
            ("dt", HtmlTag::DT),
            ("dd", HtmlTag::DD),
            ("dl", HtmlTag::DL),
            ("sup", HtmlTag::SUP),
            ("sub", HtmlTag::SUB),
            ("hr", HtmlTag::HR),
            ("b", HtmlTag::STRONG),
            ("strong", HtmlTag::STRONG),
            ("u", HtmlTag::UNDERLINE),
            ("q", HtmlTag::QUOTATION),
            ("script", HtmlTag::SCRIPT),
            ("style", HtmlTag::STYLE),
            ("table", HtmlTag::TABLE),
            ("th", HtmlTag::TH),
            ("tr", HtmlTag::TR),
            ("td", HtmlTag::TD),
            ("video", HtmlTag::VIDEO),
            ("audio", HtmlTag::AUDIO),
            ("source", HtmlTag::SOURCE),
        ]);
        Self { tags, raw_: raw }
    }

    pub fn render(
        &self,
        source: &str,
        lines: &mut Vec<(LineType, String)>,
        links: &mut Vec<LinkPair>,
        url: &str,
    ) {
        /*
         * to render the HTML, we use a self-developed "XML" pull parser.
         *
         * A pull parser works like this:
         *   - we feed it with an XML stream
         *   - we then gather an iterator
         *   - we then can iterate over all continuous elements, such as start
         * tag, close tag, text element, ...
         */
        let mut xpp = TagSoupPullParser::new(source.to_string());
        let raw_ = self.raw_;

        let mut link_num = -1;
        let mut inside_script = false;
        let mut inside_video = false;
        let mut inside_audio = false;
        let mut inside_pre = false;
        let mut pre_just_started = false;
        let mut itunes_hack = false;
        let mut curline = String::new();
        let mut indent_level = 0;
        let mut list_elements_stack = Vec::new();
        let mut inside_style = 0;

        let mut image_count = 0;
        let mut video_count = 0;
        let mut audio_count = 0;
        let mut source_count = 0;
        let mut iframe_count = 0;

        let mut pre_consecutive_nl = 0;
        let mut ol_counts = Vec::new();
        let mut ol_types = Vec::new();
        let mut tables = Vec::new();

        loop {
            let e = xpp.next();
            if e == Event::EndDocument {
                break;
            }
            if inside_script {
                // <script> tags can't be nested[1], so we simply ignore all input
                // while we're looking for the closing tag.
                //
                // 1. https://rules.sonarsource.com/html/RSPEC-4645

                if e == Event::EndTag && self.extract_tag(&mut xpp) == HtmlTag::SCRIPT {
                    inside_script = false;
                }

                // Go on to the next XML node
                continue;
            }
            match e {
                Event::StartTag => {
                    match self.extract_tag(&mut xpp) {
                        HtmlTag::A => {
                            if let Some(link) = xpp.get_attribute_value("href") {
                                if link.len() > 0 {
                                    link_num = add_link(
                                        links,
                                        &utils::censor_url(&utils::absolute_url(url, &link)),
                                        LinkType::Href,
                                    );
                                    if !raw_ {
                                        curline += "<u>";
                                    }
                                }
                            } else {
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found a tag with no href attribute"
                                );
                            }
                        }
                        HtmlTag::STRONG => {
                            if !raw_ {
                                curline += "<b>";
                            }
                        }
                        HtmlTag::UNDERLINE => {
                            if !raw_ {
                                curline += "<u>";
                            }
                        }
                        HtmlTag::QUOTATION => {
                            if !raw_ {
                                curline.push('"');
                            }
                        }
                        HtmlTag::EMBED => {
                            if let Some(type_) = xpp.get_attribute_value("type") {
                                if type_ == "application/x-shockwave-flash" {
                                    if let Some(link) = xpp.get_attribute_value("src") {
                                        if link.len() > 0 {
                                            link_num = add_link(
                                                links,
                                                &utils::censor_url(&utils::absolute_url(
                                                    url, &link,
                                                )),
                                                LinkType::Embed,
                                            );
                                            curline += &format!(
                                                "[{} {}]",
                                                gettext("embedded flash:"),
                                                link_num
                                            );
                                        }
                                    } else {
                                        log!(Level::Warn, "HtmlRenderer::render: found embed object without src attribute");
                                    }
                                }
                            } else {
                                log!(Level::Warn, "HtmlRenderer::render: found embed object without type attribute");
                            }
                        }
                        HtmlTag::IFRAME => {
                            let iframe_url = xpp.get_attribute_value("src").unwrap_or_else(|| {
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found iframe tag without src attribute"
                                );
                                String::new()
                            });

                            let iframe_title = xpp.get_attribute_value("title").unwrap_or_default();

                            if !iframe_url.is_empty() {
                                add_nonempty_line(&curline, &mut tables, lines);
                                if let Some(last) = lines.last() {
                                    if last.1.len() > (indent_level as usize * 2) {
                                        add_line("", &mut tables, lines);
                                    }
                                }

                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });

                                iframe_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &iframe_url,
                                    &iframe_title,
                                    iframe_count,
                                    LinkType::Iframe,
                                );
                                add_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                        }
                        HtmlTag::BR => {
                            add_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::PRE => {
                            inside_pre = true;
                            pre_just_started = true;
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::ITUNESHACK => {
                            itunes_hack = true;
                        }
                        HtmlTag::IMG => {
                            let img_url = xpp.get_attribute_value("src").unwrap_or_else(|| {
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found img tag with no src attribute"
                                );
                                String::new()
                            });

                            // Prefer `alt' over `title'
                            let mut img_label = xpp.get_attribute_value("alt").unwrap_or_default();
                            if img_label.is_empty() {
                                if let Some(title) = xpp.get_attribute_value("title") {
                                    img_label = title;
                                }
                            }
                            if !img_url.is_empty() {
                                image_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &img_url,
                                    &img_label,
                                    image_count,
                                    LinkType::Img,
                                );
                            }
                        }
                        HtmlTag::BLOCKQUOTE => {
                            indent_level += 1;
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }

                        HtmlTag::H1
                        | HtmlTag::H2
                        | HtmlTag::H3
                        | HtmlTag::H4
                        | HtmlTag::H5
                        | HtmlTag::H6
                        | HtmlTag::P
                        | HtmlTag::DIV => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            if let Some((_, last_line)) = lines.last() {
                                if last_line.len() > (indent_level * 2) as usize {
                                    add_line("", &mut tables, lines);
                                }
                            }
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::OL => {
                            list_elements_stack.push(HtmlTag::OL);

                            let ol_count_str =
                                xpp.get_attribute_value("start").unwrap_or("1".to_string());
                            let ol_count = utils::to_u(ol_count_str, 1);
                            ol_counts.push(ol_count);

                            let mut ol_type =
                                xpp.get_attribute_value("type").unwrap_or("1".to_string());
                            if ol_type != "1"
                                && ol_type != "a"
                                && ol_type != "A"
                                && ol_type != "i"
                                && ol_type != "I"
                            {
                                ol_type = "1".to_string();
                            }
                            // TODO: could the attribute value be empty?
                            ol_types
                                .push(ol_type.chars().next().expect("non empty ol type attribute"));

                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::UL => {
                            list_elements_stack.push(HtmlTag::UL);
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::LI => {
                            if list_elements_stack.last() == Some(&HtmlTag::LI) {
                                list_elements_stack.pop();
                                indent_level -= 2;
                                if indent_level < 0 {
                                    indent_level = 0;
                                }
                                add_nonempty_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }

                            list_elements_stack.push(HtmlTag::LI);
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                            indent_level += 2;

                            let latest_list = list_elements_stack
                                .iter()
                                .rev()
                                .find(|&tag| (tag == &HtmlTag::OL || tag == &HtmlTag::UL));
                            let inside_ordered_list = latest_list == Some(&HtmlTag::OL);

                            if inside_ordered_list && !ol_counts.is_empty() {
                                curline += &format!(
                                    "{}. ",
                                    format_ol_count(
                                        ol_counts[ol_counts.len() - 1],
                                        ol_types[ol_types.len() - 1],
                                    )
                                );
                                let idx = ol_counts.len() - 1;
                                ol_counts[idx] += 1;
                            } else {
                                curline += "  * ";
                            }
                        }
                        HtmlTag::DT => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::DD => {
                            indent_level += 4;
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::DL => {
                            // ignore tag
                        }
                        HtmlTag::SUP => {
                            curline.push('^');
                        }
                        HtmlTag::SUB => {
                            curline.push('[');
                        }
                        HtmlTag::HR => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                            add_hr(lines);
                        }
                        HtmlTag::SCRIPT => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });

                            // don't render scripts, ignore current line
                            inside_script = true;
                        }
                        HtmlTag::STYLE => {
                            inside_style += 1;
                        }
                        HtmlTag::TABLE => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = prepare_new_line(0); // no indent in tables

                            let has_border = xpp
                                .get_attribute_value("border")
                                .map(|b| utils::to_u(b, 0) > 0)
                                .unwrap_or(false);
                            tables.push(Table::new(has_border));
                        }
                        HtmlTag::TR => {
                            if let Some(last) = tables.last_mut() {
                                last.start_row();
                            }
                        }
                        HtmlTag::TH => {
                            let span = xpp
                                .get_attribute_value("colspan")
                                .map(|colspan| utils::to_u(colspan, 1))
                                .unwrap_or(1);
                            if let Some(last) = tables.last_mut() {
                                last.start_cell(span);
                            }
                            curline += "<b>";
                        }
                        HtmlTag::TD => {
                            let span = xpp
                                .get_attribute_value("colspan")
                                .map(|colspan| utils::to_u(colspan, 1))
                                .unwrap_or(1);
                            if let Some(last) = tables.last_mut() {
                                last.start_cell(span);
                            }
                        }
                        HtmlTag::VIDEO => {
                            // Decrement the appropriate counter if the
                            // previous media element had no sources
                            if inside_video && source_count == 0 {
                                video_count -= 1;
                            }
                            if inside_audio && source_count == 0 {
                                audio_count -= 1;
                            }

                            // "Pop" the previous media element if it didn't
                            // have a closing tag
                            if inside_video || inside_audio {
                                source_count = 0;
                                inside_audio = false;
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: media element left unclosed"
                                );
                            }
                            inside_video = true;

                            // We will decrement this counter later if the
                            // current <video> element contains no sources,
                            // either by its `src' attribute or any child
                            // <source> elements
                            video_count += 1;

                            let video_url = xpp.get_attribute_value("src").unwrap_or_default();

                            if !video_url.is_empty() {
                                // Video source retrieved from `src'
                                // attribute
                                source_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &video_url,
                                    "",
                                    video_count,
                                    LinkType::Video,
                                );

                                add_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                        }
                        HtmlTag::AUDIO => {
                            // Decrement the appropriate counter if the
                            // previous media element had no sources
                            if inside_video && source_count == 0 {
                                video_count -= 1;
                            }
                            if inside_audio && source_count == 0 {
                                audio_count -= 1;
                            }

                            // "Pop" the previous media element if it didn't
                            // have a closing tag
                            if inside_video || inside_audio {
                                source_count = 0;
                                inside_video = false;
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: media element left unclosed"
                                );
                            }
                            inside_audio = true;

                            // We will decrement this counter later if the
                            // current <audio> element contains no sources,
                            // either by its `src' attribute or any child
                            // <source> elements
                            audio_count += 1;

                            let audio_url = xpp.get_attribute_value("src").unwrap_or_default();
                            if !audio_url.is_empty() {
                                // Audio source retrieved from `src'
                                // attribute
                                source_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &audio_url,
                                    "",
                                    audio_count,
                                    LinkType::Audio,
                                );

                                add_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                        }
                        HtmlTag::SOURCE => {
                            let source_url = xpp.get_attribute_value("src").unwrap_or_else(|| {
                                log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found source tag with no src attribute"
                                );
                                String::new()
                            });

                            if inside_video && !source_url.is_empty() {
                                source_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &source_url,
                                    "",
                                    video_count,
                                    LinkType::Video,
                                );

                                add_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }

                            if inside_audio && !source_url.is_empty() {
                                source_count += 1;
                                curline += &add_media_link(
                                    links,
                                    url,
                                    &source_url,
                                    "",
                                    audio_count,
                                    LinkType::Audio,
                                );

                                add_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                        }

                        _ => {}
                    }
                }
                Event::EndTag => {
                    let tag = self.extract_tag(&mut xpp);
                    match tag {
                        HtmlTag::BLOCKQUOTE => {
                            indent_level -= 1;
                            if indent_level < 0 {
                                indent_level = 0;
                            }
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::OL | HtmlTag::UL => {
                            if tag == HtmlTag::OL && !ol_types.is_empty() {
                                ol_types.pop();
                                ol_counts.pop();
                            }
                            if list_elements_stack.last() == Some(&HtmlTag::LI) {
                                list_elements_stack.pop();
                                indent_level -= 2;
                                if indent_level < 0 {
                                    indent_level = 0;
                                }
                                add_nonempty_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                            list_elements_stack.pop();
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::DT => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::DD => {
                            indent_level -= 4;
                            if indent_level < 0 {
                                indent_level = 0;
                            }
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::DL => {
                            // ignore tag
                        }
                        HtmlTag::LI => {
                            indent_level -= 2;
                            if indent_level < 0 {
                                indent_level = 0;
                            }
                            if list_elements_stack.last() == Some(&HtmlTag::LI) {
                                list_elements_stack.pop();
                            }
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::H1 => {
                            if line_is_nonempty(&curline) {
                                add_line(&curline, &mut tables, lines);
                                let llen = utils::strwidth_stfl(&curline);
                                add_line(&"-".repeat(llen), &mut tables, lines);
                            }
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::H2
                        | HtmlTag::H3
                        | HtmlTag::H4
                        | HtmlTag::H5
                        | HtmlTag::H6
                        | HtmlTag::P
                        | HtmlTag::DIV => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::PRE => {
                            if pre_consecutive_nl > 1 {
                                lines.pop();
                            } else if pre_consecutive_nl == 0 {
                                add_line_softwrappable(&curline, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                            }
                            inside_pre = false;
                            pre_just_started = false;
                            pre_consecutive_nl = 0;
                        }
                        HtmlTag::SUB => {
                            curline.push(']');
                        }
                        HtmlTag::SUP => {
                            // has closing tag, but we render nothing.
                        }
                        HtmlTag::A => {
                            if link_num != -1 {
                                if !raw_ {
                                    curline += "</>";
                                }
                                curline += &format!("[{}]", link_num);
                                link_num = -1;
                            }
                        }
                        HtmlTag::UNDERLINE => {
                            if !raw_ {
                                curline += "</>";
                            }
                        }
                        HtmlTag::STRONG => {
                            if !raw_ {
                                curline += "</>";
                            }
                        }
                        HtmlTag::QUOTATION => {
                            if !raw_ {
                                curline.push('"');
                            }
                        }
                        HtmlTag::EMBED
                        | HtmlTag::IFRAME
                        | HtmlTag::BR
                        | HtmlTag::ITUNESHACK
                        | HtmlTag::IMG
                        | HtmlTag::HR
                        | HtmlTag::SOURCE => {
                            // ignore closing tags
                        }
                        HtmlTag::SCRIPT => {
                            // This line is unreachable, since we handle closing <script>
                            // tags way before this entire `switch`.
                        }
                        HtmlTag::STYLE => {
                            if inside_style > 0 {
                                inside_style -= 1;
                            }
                        }
                        HtmlTag::TABLE => {
                            add_nonempty_line(&curline, &mut tables, lines);

                            if let Some(last) = tables.last_mut() {
                                let mut table_text = Vec::new();
                                last.complete_cell();
                                last.complete_row();
                                render_table(last, &mut table_text);
                                tables.pop();

                                // still a table on the outside?
                                if let Some(last) = tables.last_mut() {
                                    for (_, s) in table_text {
                                        // add rendered table to current cell
                                        last.add_text(&s);
                                    }
                                } else {
                                    for (_, s) in table_text {
                                        let mut s: &str = &s;
                                        while s.starts_with('\n') {
                                            s = &s[1..]
                                        }
                                        add_line_nonwrappable(s, lines);
                                    }
                                }
                            }
                            curline =
                                prepare_new_line(if !tables.is_empty() { 0 } else { indent_level });
                        }
                        HtmlTag::TR => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = prepare_new_line(0); // no indent in tables
                                                           //
                            if let Some(last) = tables.last_mut() {
                                last.complete_row();
                            }
                        }
                        HtmlTag::TH => {
                            if !tables.is_empty() {
                                curline += "</>";
                            }

                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = prepare_new_line(0); // no indent in tables

                            if let Some(last) = tables.last_mut() {
                                last.complete_cell();
                            }
                        }
                        HtmlTag::TD => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = prepare_new_line(0); // no indent in tables

                            if let Some(last) = tables.last_mut() {
                                last.complete_cell();
                            }
                        }
                        HtmlTag::VIDEO => {
                            if inside_video && source_count == 0 {
                                video_count -= 1;
                            }

                            inside_video = false;
                            source_count = 0;
                        }
                        HtmlTag::AUDIO => {
                            if inside_audio && source_count == 0 {
                                audio_count -= 1;
                            }

                            inside_audio = false;
                            source_count = 0;
                        }

                        _ => {}
                    }
                }
                Event::Text => {
                    let mut text = xpp.get_text();
                    if !raw_ {
                        text = utils::quote_for_stfl(&text);
                    }
                    if itunes_hack {
                        let paragraphs = utils::tokenize_nl(&text, None);
                        for paragraph in paragraphs {
                            if paragraph != "\n" {
                                add_nonempty_line(&curline, &mut tables, lines);
                                curline = prepare_new_line(if !tables.is_empty() {
                                    0
                                } else {
                                    indent_level
                                });
                                curline += &paragraph;
                            }
                        }
                    } else if inside_pre {
                        let delims = "\r\n";
                        let paragraphs = utils::tokenize_spaced(&text, Some(delims));
                        for paragraph in paragraphs {
                            if paragraph.find(|c| !delims.contains(c)).is_none() {
                                for i in 0..paragraph.len() {
                                    if pre_just_started && i == 0 {
                                        continue;
                                    }
                                    add_line_softwrappable(&curline, lines);
                                    curline = prepare_new_line(if !tables.is_empty() {
                                        0
                                    } else {
                                        indent_level
                                    });
                                    pre_consecutive_nl += 1;
                                }
                            } else {
                                curline += &paragraph;
                                pre_consecutive_nl = 0;
                            }
                            pre_just_started = false;
                        }
                    } else if inside_style > 0 || inside_video || inside_audio {
                        // skip scripts, CSS styles and fallback text for media elements
                    } else {
                        // strip leading whitespace
                        let before = text.len();
                        text = text.trim_start().to_string();
                        let had_whitespace = text.len() < before;
                        if line_is_nonempty(&curline) && had_whitespace {
                            curline.push(' ');
                        }
                        // strip newlines
                        text = utils::replace_all(text, "\n", " ");
                        curline += &text;
                    }
                }
                _ => {
                    // do nothing
                }
            }
        }

        // remove trailing line break if <pre> is still open
        if pre_consecutive_nl > 1 {
            lines.pop();
        }

        // and the rest
        add_nonempty_line(&curline, &mut tables, lines);

        // force all tables to be closed and rendered
        while let Some(last) = tables.last() {
            let mut table_text = Vec::new();
            render_table(last, &mut table_text);
            tables.pop();
            for (_, s) in &mut table_text {
                while s.starts_with('\n') {
                    s.remove(0);
                }
                add_line_nonwrappable(&s, lines);
            }
        }

        // add link list
        if !links.is_empty() {
            add_line("", &mut tables, lines);
            add_line(&gettext("Links: "), &mut tables, lines);
            for (i, link) in links.iter().enumerate() {
                let link_text = format!("[{}]: {} ({})", i + 1, link.0, type2str(link.1));
                add_line_softwrappable(&link_text, lines);
            }
        }
    }

    pub fn render_hr(width: u32) -> String {
        assert!(width >= 2);
        let mut result = "\n ".to_string();
        result += &"-".repeat((width - 2) as usize);
        result += " \n";
        result
    }

    fn extract_tag(&self, parser: &mut TagSoupPullParser) -> HtmlTag {
        let tagname = parser.get_text().to_lowercase();
        *self.tags.get(tagname.as_str()).unwrap_or(&HtmlTag::Unknown)
    }
}

fn add_media_link(
    links: &mut Vec<(String, LinkType)>,
    url: &str,
    media_url: &str,
    media_title: &str,
    media_count: u32,
    ty: LinkType,
) -> String {
    // Display media_url except if it's an inline image
    let link_url = if ty == LinkType::Img && media_url.starts_with("data:") {
        "inline image".to_string()
    } else {
        utils::censor_url(&utils::absolute_url(url, media_url))
    };

    let type_str = type2str(ty);
    let link_num = add_link(links, &link_url, ty);

    if !media_title.is_empty() {
        format!(
            "[{} {}: {} ({} #{})]",
            gettext(&type_str),
            media_count,
            media_title,
            gettext("link"),
            link_num
        )
    } else {
        format!(
            "[{} {} ({} #{})]",
            gettext(&type_str),
            media_count,
            gettext("link"),
            link_num
        )
    }
}

fn render_table(table: &Table, lines: &mut Vec<(LineType, String)>) {
    // get maximum number of cells
    let mut cells: usize = 0;
    for row in &table.rows {
        let mut count = 0;
        for cell in &row.cells {
            count += cell.span;
        }
        cells = cells.max(count as usize);
    }

    // get width of each row
    let mut cell_widths = Vec::new();
    cell_widths.resize(cells, 0);
    for row in &table.rows {
        for (i, cell) in row.cells.iter().enumerate() {
            let mut width = 0;
            for text in &cell.text {
                width = width.max(utils::strwidth_stfl(text));
            }
            let span = cell.span as usize;
            if span > 1 {
                width += span;
                // divide size evenly on columns (can be done better, I know)
                width /= span;
            }
            cell_widths[i] = cell_widths[i].max(width);
        }
    }

    let hsep = '-';
    let mut vsep = '|';
    let hvsep = '+';
    //
    // create a row separator
    let mut separator = String::new();
    if table.has_border {
        separator.push(hvsep);
    }
    for width in &cell_widths {
        separator += &hsep.to_string().repeat(*width);
        separator.push(hvsep);
    }

    if !table.has_border {
        vsep = ' ';
    }

    // render the table
    if table.has_border {
        lines.push((LineType::Nonwrappable, separator.clone()));
    }

    for (row, _) in table.rows.iter().enumerate() {
        // calc height of this row
        let mut height = 0;
        for cell in &table.rows[row].cells {
            height = height.max(cell.text.len());
        }

        for idx in 0..height {
            let mut line = String::new();
            if table.has_border {
                line.push(vsep);
            }

            for (cell, _) in table.rows[row].cells.iter().enumerate() {
                let mut cell_width = 0;
                if idx < table.rows[row].cells[cell].text.len() {
                    log!(
                        Level::Debug,
                        "row = {} cell = {} text = {}",
                        row,
                        cell,
                        table.rows[row].cells[cell].text[idx]
                    );
                    cell_width = utils::strwidth_stfl(&table.rows[row].cells[cell].text[idx]);
                    line += &table.rows[row].cells[cell].text[idx];
                }

                let mut reference_width = cell_widths[cell];
                if table.rows[row].cells[cell].span > 1 {
                    for ic in cell + 1..cell + table.rows[row].cells[cell].span as usize {
                        reference_width += cell_widths[ic] + 1;
                    }
                }

                log!(
                    Level::Debug,
                    "cell_width = {} reference_width = {}",
                    cell_width,
                    reference_width
                );
                if cell_width < reference_width {
                    // pad, if necessary
                    line += &" ".repeat(reference_width - cell_width);
                }

                if cell < table.rows[row].cells.len() - 1 {
                    line.push(vsep);
                }
            }
            if table.has_border {
                line.push(vsep);
            }
            lines.push((LineType::Nonwrappable, line));
        }
        if table.has_border {
            lines.push((LineType::Nonwrappable, separator.clone()));
        }
    }
}

fn add_line_nonwrappable(line: &str, lines: &mut Vec<(LineType, String)>) {
    lines.push((LineType::Nonwrappable, line.to_string()));
}

fn add_line_softwrappable(line: &str, lines: &mut Vec<(LineType, String)>) {
    lines.push((LineType::Softwrappable, line.to_string()));
}

fn add_hr(lines: &mut Vec<(LineType, String)>) {
    lines.push((LineType::Hr, String::new()));
}

fn add_nonempty_line(line: &str, tables: &mut Vec<Table>, lines: &mut Vec<(LineType, String)>) {
    if line_is_nonempty(line) {
        add_line(line, tables, lines);
    }
}

fn line_is_nonempty(line: &str) -> bool {
    line.chars()
        .any(|c| !c.is_whitespace() && c != '\n' && c != '\r')
}

fn prepare_new_line(indent_level: i32) -> String {
    " ".repeat(0.max(indent_level) as usize * 2)
}

fn add_line(curline: &str, tables: &mut Vec<Table>, lines: &mut Vec<(LineType, String)>) {
    if let Some(last) = tables.last_mut() {
        last.add_text(curline);
    } else {
        lines.push((LineType::Wrappable, curline.to_string()));
    }
}

fn add_link(links: &mut Vec<(String, LinkType)>, link: &str, href: LinkType) -> i32 {
    let mut i = 1;
    for (v, _) in links.iter() {
        if v == link {
            return i;
        }
        i += 1;
    }
    links.push((link.to_string(), href));
    i
}

fn type2str(ty: LinkType) -> String {
    match ty {
        LinkType::Href => gettext("link"),
        LinkType::Img => gettext("image"),
        LinkType::Embed => gettext("embedded flash"),
        LinkType::Iframe => gettext("iframe"),
        LinkType::Video => gettext("video"),
        LinkType::Audio => gettext("audio"),
    }
}

fn format_ol_count(count: u32, ty: char) -> String {
    match ty {
        'a' => get_char_numbering(count),
        'A' => get_char_numbering(count).to_uppercase(),
        'i' => get_roman_numbering(count),
        'I' => get_roman_numbering(count).to_uppercase(),
        '1' | _ => format!("{:2}", count),
    }
}

fn get_roman_numbering(count: u32) -> String {
    let values = [1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1];
    let numerals = [
        "m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i",
    ];
    let mut result = String::new();
    let mut count = count;
    for (value, numeral) in values.iter().zip(numerals) {
        while count >= *value {
            count -= value;
            result += numeral;
        }
    }
    result
}

fn get_char_numbering(count: u32) -> String {
    let mut result = String::new();
    let mut count = count;

    loop {
        count -= 1;
        result.push(char::from_u32('a' as u32 + count % 26).unwrap());
        count /= 26;
        if count == 0 {
            break;
        }
    }

    result.chars().rev().collect()
}
