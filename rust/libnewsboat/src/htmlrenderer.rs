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
        let mut xpp = TagSoupPullParser::new(source);
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

        macro_rules! indent {
            () => {
                indent!(if !tables.is_empty() { 0 } else { indent_level })
            };
            ($value:expr) => {
                prepare_new_line($value)
            };
        }

        while let Some(e) = xpp.next() {
            if inside_script {
                // <script> tags can't be nested[1], so we simply ignore all input
                // while we're looking for the closing tag.
                //
                // 1. https://rules.sonarsource.com/html/RSPEC-4645

                if let Event::EndTag(tag) = &e {
                    if self.extract_tag(&tag) == HtmlTag::SCRIPT {
                        inside_script = false;
                    }
                }

                // Go on to the next XML node
                continue;
            }
            match e {
                Event::StartTag(tag, attr) => {
                    match self.extract_tag(&tag) {
                        HtmlTag::A => {
                            if let Some(link) = attr.get("href") {
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
                            if let Some("application/x-shockwave-flash") =
                                attr.get("type").map(|s| s.as_str())
                            {
                                if let Some(link) = attr.get("src") {
                                    if !link.is_empty() {
                                        link_num = add_link(
                                            links,
                                            &utils::censor_url(&utils::absolute_url(url, &link)),
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
                            } else {
                                log!(Level::Warn, "HtmlRenderer::render: found embed object without type attribute");
                            }
                        }
                        HtmlTag::IFRAME => {
                            let iframe_url =
                                attr.get("src").map(|s| s.as_str()).unwrap_or_else(|| {
                                    log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found iframe tag without src attribute"
                                );
                                    ""
                                });

                            let iframe_title =
                                attr.get("title").map(|s| s.as_str()).unwrap_or_default();

                            if !iframe_url.is_empty() {
                                add_nonempty_line(&curline, &mut tables, lines);
                                if let Some(last) = lines.last() {
                                    if last.1.len() > (indent_level as usize * 2) {
                                        add_line("", &mut tables, lines);
                                    }
                                }

                                curline = indent!();

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
                                curline = indent!();
                            }
                        }
                        HtmlTag::BR => {
                            add_line(&curline, &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::PRE => {
                            inside_pre = true;
                            pre_just_started = true;
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::ITUNESHACK => {
                            itunes_hack = true;
                        }
                        HtmlTag::IMG => {
                            let img_url =
                                attr.get("src").map(|s| s.as_str()).unwrap_or_else(|| {
                                    log!(
                                        Level::Warn,
                                        "HtmlRenderer::render: found img tag with no src attribute"
                                    );
                                    ""
                                });

                            // Prefer `alt' over `title'
                            let mut img_label =
                                attr.get("alt").map(|s| s.as_str()).unwrap_or_default();
                            if img_label.is_empty() {
                                if let Some(title) = attr.get("title") {
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
                            curline = indent!();
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
                            curline = indent!();
                        }
                        HtmlTag::OL => {
                            list_elements_stack.push(HtmlTag::OL);

                            let ol_count_str = attr.get("start").map(|s| s.as_str()).unwrap_or("1");
                            let ol_count = ol_count_str.parse().unwrap_or(1);
                            ol_counts.push(ol_count);

                            let mut ol_type = attr.get("type").map(|s| s.as_str()).unwrap_or("1");
                            if ol_type != "1"
                                && ol_type != "a"
                                && ol_type != "A"
                                && ol_type != "i"
                                && ol_type != "I"
                            {
                                ol_type = "1";
                            }
                            // TODO: could the attribute value be empty?
                            ol_types
                                .push(ol_type.chars().next().expect("non empty ol type attribute"));

                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::UL => {
                            list_elements_stack.push(HtmlTag::UL);
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::LI => {
                            if list_elements_stack.last() == Some(&HtmlTag::LI) {
                                list_elements_stack.pop();
                                indent_level -= 2;
                                if indent_level < 0 {
                                    indent_level = 0;
                                }
                                add_nonempty_line(&curline, &mut tables, lines);
                                curline = indent!();
                            }

                            list_elements_stack.push(HtmlTag::LI);
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!();
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
                            curline = indent!();
                        }
                        HtmlTag::DD => {
                            indent_level += 4;
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!();
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
                            curline = indent!();
                            add_hr(lines);
                        }
                        HtmlTag::SCRIPT => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!();

                            // don't render scripts, ignore current line
                            inside_script = true;
                        }
                        HtmlTag::STYLE => {
                            inside_style += 1;
                        }
                        HtmlTag::TABLE => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!(0); // no indent in tables

                            let has_border = attr
                                .get("border")
                                .map(|b| b.parse::<u32>().unwrap_or(0) > 0)
                                .unwrap_or(false);
                            tables.push(Table::new(has_border));
                        }
                        HtmlTag::TR => {
                            if let Some(last) = tables.last_mut() {
                                last.start_row();
                            }
                        }
                        HtmlTag::TH => {
                            let span = attr
                                .get("colspan")
                                .and_then(|colspan| colspan.parse().ok())
                                .unwrap_or(1);
                            if let Some(last) = tables.last_mut() {
                                last.start_cell(span);
                            }
                            curline += "<b>";
                        }
                        HtmlTag::TD => {
                            let span = attr
                                .get("colspan")
                                .and_then(|colspan| colspan.parse().ok())
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

                            let video_url = attr.get("src").map(|s| s.as_str()).unwrap_or_default();
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
                                curline = indent!();
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

                            let audio_url = attr.get("src").map(|s| s.as_str()).unwrap_or_default();
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
                                curline = indent!();
                            }
                        }
                        HtmlTag::SOURCE => {
                            let source_url =
                                attr.get("src").map(|s| s.as_str()).unwrap_or_else(|| {
                                    log!(
                                    Level::Warn,
                                    "HtmlRenderer::render: found source tag with no src attribute"
                                );
                                    ""
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
                                curline = indent!();
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
                                curline = indent!();
                            }
                        }

                        _ => {}
                    }
                }
                Event::EndTag(tag) => {
                    let tag = self.extract_tag(&tag);
                    match tag {
                        HtmlTag::BLOCKQUOTE => {
                            indent_level -= 1;
                            if indent_level < 0 {
                                indent_level = 0;
                            }
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
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
                                curline = indent!();
                            }
                            list_elements_stack.pop();
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::DT => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::DD => {
                            indent_level -= 4;
                            if indent_level < 0 {
                                indent_level = 0;
                            }
                            add_nonempty_line(&curline, &mut tables, lines);
                            add_line("", &mut tables, lines);
                            curline = indent!();
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
                            curline = indent!();
                        }
                        HtmlTag::H1 => {
                            if line_is_nonempty(&curline) {
                                add_line(&curline, &mut tables, lines);
                                let llen = utils::strwidth_stfl(&curline);
                                add_line(&"-".repeat(llen), &mut tables, lines);
                            }
                            curline = indent!();
                        }
                        HtmlTag::H2
                        | HtmlTag::H3
                        | HtmlTag::H4
                        | HtmlTag::H5
                        | HtmlTag::H6
                        | HtmlTag::P
                        | HtmlTag::DIV => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!();
                        }
                        HtmlTag::PRE => {
                            if pre_consecutive_nl > 1 {
                                lines.pop();
                            } else if pre_consecutive_nl == 0 {
                                add_line_softwrappable(&curline, lines);
                                curline = indent!();
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
                            curline = indent!();
                        }
                        HtmlTag::TR => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!(0); // no indent in tables
                            if let Some(last) = tables.last_mut() {
                                last.complete_row();
                            }
                        }
                        HtmlTag::TH => {
                            if !tables.is_empty() {
                                curline += "</>";
                            }

                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!(0); // no indent in tables

                            if let Some(last) = tables.last_mut() {
                                last.complete_cell();
                            }
                        }
                        HtmlTag::TD => {
                            add_nonempty_line(&curline, &mut tables, lines);
                            curline = indent!(0); // no indent in tables

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
                Event::Text(mut text) => {
                    if !raw_ {
                        text = utils::quote_for_stfl(&text);
                    }
                    if itunes_hack {
                        let paragraphs = utils::tokenize_nl(&text, None);
                        for paragraph in paragraphs {
                            if paragraph != "\n" {
                                add_nonempty_line(&curline, &mut tables, lines);
                                curline = indent!();
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
                                    curline = indent!();
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
                        text = text.replace("\n", " ");
                        curline += &text;
                    }
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

    fn extract_tag(&self, tag: &str) -> HtmlTag {
        let tagname = tag.to_lowercase();
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
    let mut cells = 0;
    for row in &table.rows {
        cells = cells.max(row.cells.iter().map(|cell| cell.span).sum());
    }

    // get width of each row
    let mut cell_widths = vec![0; cells as usize];
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
    let vsep = if table.has_border { '|' } else { ' ' };
    let hvsep = '+';

    // create a row separator
    let mut separator = String::new();
    if table.has_border {
        separator.push(hvsep);
    }
    for width in &cell_widths {
        separator += &hsep.to_string().repeat(*width);
        separator.push(hvsep);
    }

    // render the table
    if table.has_border {
        lines.push((LineType::Nonwrappable, separator.clone()));
    }

    for (i, row) in table.rows.iter().enumerate() {
        // calc height of this row
        let mut height = 0;
        for cell in &row.cells {
            height = height.max(cell.text.len());
        }

        for idx in 0..height {
            let mut line = String::new();
            if table.has_border {
                line.push(vsep);
            }

            for (j, cell) in row.cells.iter().enumerate() {
                let mut cell_width = 0;
                if idx < cell.text.len() {
                    log!(
                        Level::Debug,
                        "row = {} cell = {} text = {}",
                        i,
                        j,
                        cell.text[idx]
                    );
                    cell_width = utils::strwidth_stfl(&cell.text[idx]);
                    line += &cell.text[idx];
                }

                let mut reference_width = cell_widths[j];
                if row.cells[j].span > 1 {
                    for ic in j + 1..j + cell.span as usize {
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

                if j < row.cells.len() - 1 {
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
    line.chars().any(|c| !c.is_whitespace())
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

#[cfg(test)]
mod tests {
    use super::*;

    const URL: &str = "http://example.com/feed.rss";

    macro_rules! line {
        ($ty:ident, $content:literal) => {
            (LineType::$ty, $content.to_string())
        };
    }

    macro_rules! link {
        ($content:literal, $ty:ident) => {
            ($content.to_string(), LinkType::$ty)
        };
    }

    macro_rules! render {
        ($source:expr) => {
            render!($source, "")
        };
        ($source:expr, $url:expr) => {{
            let rnd = HtmlRenderer::new(false);
            let mut lines = Vec::new();
            let mut links = Vec::new();
            rnd.render($source, &mut lines, &mut links, $url);
            (lines, links)
        }};
    }

    #[test]
    fn t_htmlrenderer_links_are_rendered_as_underlined_text_with_reference_number_in_square_brackets(
    ) {
        let (lines, links) = render!("<a href=\"http://slashdot.org/\">slashdot</a>");

        assert_eq!(
            lines,
            [
                line!(Wrappable, "<u>slashdot</>[1]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://slashdot.org/ (link)")
            ]
        );

        assert_eq!(links, [link!("http://slashdot.org/", Href)]);
    }

    #[test]
    fn t_htmlrenderer_br_tag_results_in_a_line_break() {
        for tag in ["<br>", "<br/>", "<br />"] {
            let input = format!("hello{}world!", &tag);
            let (lines, _) = render!(&input);
            assert_eq!(
                lines,
                [line!(Wrappable, "hello"), line!(Wrappable, "world!"),]
            );
        }
    }

    #[test]
    fn t_htmlrenderer_superscript_is_rendered_with_caret_symbol() {
        let (lines, _) = render!("3<sup>10</sup>");
        assert_eq!(lines, [line!(Wrappable, "3^10")]);
    }

    #[test]
    fn t_htmlrenderer_subscript_is_rendered_with_square_brackets() {
        let (lines, _) = render!("A<sub>i</sub>");
        assert_eq!(lines, [line!(Wrappable, "A[i]")]);
    }

    #[test]
    fn t_htmlrenderer_script_tags_are_ignored() {
        let (lines, _) = render!("abc<script></script>");
        assert_eq!(lines, [line!(Wrappable, "abc")]);
    }

    macro_rules! check_format_ol_count {
        ($name:ident, $format:literal, $cases:expr) => {
            #[test]
            fn $name() {
                for (count, expected) in $cases {
                    assert_eq!(format_ol_count(count, $format), expected);
                }
            }
        };
    }

    check_format_ol_count!(
        t_htmlrenderer_format_ol_count_formats_list_count_to_digit,
        '1',
        [(1, " 1"), (3, " 3")]
    );

    check_format_ol_count!(
        t_htmlrenderer_format_ol_count_formats_list_count_to_alphabetic,
        'a',
        [(3, "c"), (26 + 3, "ac"), (3 * 26 * 26 + 5 * 26 + 2, "ceb")]
    );

    check_format_ol_count!(
        t_htmlrenderer_format_ol_count_formats_list_count_to_alphabetic_uppercase,
        'A',
        [
            (3, "C"),
            (26 + 5, "AE"),
            (27, "AA"),
            (26, "Z"),
            (26 * 26 + 26, "ZZ"),
            (25 * 26 * 26 + 26 * 26 + 26, "YZZ"),
        ]
    );

    check_format_ol_count!(
        t_htmlrenderer_format_ol_count_formats_list_count_to_roman_numerals,
        'i',
        [
            (1, "i"),
            (2, "ii"),
            (5, "v"),
            (4, "iv"),
            (6, "vi"),
            (7, "vii"),
            (10, "x"),
            (32, "xxxii"),
            (1972, "mcmlxxii"),
        ]
    );

    check_format_ol_count!(
        t_htmlrenderer_format_ol_count_formats_list_count_to_roman_numerals_uppercase,
        'I',
        [(2011, "MMXI")]
    );

    #[test]
    fn t_htmlrenderer_links_with_same_url_are_coalesced_under_one_number() {
        let input = concat!(
            "<a href='http://example.com/about'>About us</a>",
            "<a href='http://example.com/about'>Another link</a>"
        );

        let (_, links) = render!(input, URL);
        assert_eq!(links, [link!("http://example.com/about", Href)]);
    }

    #[test]
    fn t_htmlrenderer_links_with_different_urls_have_different_numbers() {
        let input = concat!(
            "<a href='http://example.com/one'>One</a>",
            "<a href='http://example.com/two'>Two</a>"
        );

        let (_, links) = render!(input, URL);

        assert_eq!(
            links,
            [
                link!("http://example.com/one", Href),
                link!("http://example.com/two", Href),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_link_without_href_is_neither_highlighted_nor_added_to_links_list() {
        let input = "<a>test</a>";
        let (lines, links) = render!(input, URL);

        assert_eq!(lines, [line!(Wrappable, "test")]);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_link_with_empty_href_is_neither_highlighted_nor_added_to_links_list() {
        let input = "<a href=''>test</a>";
        let (lines, links) = render!(input, URL);

        assert_eq!(lines, [line!(Wrappable, "test")]);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_strong_is_rendered_in_bold_font() {
        let input = "<strong>test</strong>";
        let (lines, _) = render!(input, URL);
        assert_eq!(lines, [line!(Wrappable, "<b>test</>")]);
    }

    #[test]
    fn t_htmlrenderer_u_is_rendered_as_underlined_text() {
        let input = "<u>test</u>";
        let (lines, _) = render!(input, URL);
        assert_eq!(lines, [line!(Wrappable, "<u>test</>")]);
    }

    #[test]
    fn t_htmlrenderer_q_is_rendered_as_text_in_quotes() {
        let input = "<q>test</q>";
        let (lines, _) = render!(input, URL);
        assert_eq!(lines, [line!(Wrappable, "\"test\"")]);
    }

    #[test]
    fn t_htmlrenderer_flash_embed_s_are_added_to_links_if_src_is_set() {
        let input = concat!(
            "<embed type='application/x-shockwave-flash'",
            "src='http://example.com/game.swf'>",
            "</embed>"
        );
        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[embedded flash: 1]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(
                    Softwrappable,
                    "[1]: http://example.com/game.swf (embedded flash)"
                ),
            ]
        );

        assert_eq!(links, [link!("http://example.com/game.swf", Embed)]);
    }

    #[test]
    fn t_htmlrenderer_flash_embed_s_are_ignored_if_src_is_not_set() {
        let input = "<embed type='application/x-shockwave-flash'></embed>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_non_flash_embed_s_are_ignored() {
        let input = concat!(
            "<embed type='whatever'",
            "src='http://example.com/thingy'></embed>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_embed_s_are_ignored_if_type_is_not_set() {
        let input = "<embed src='http://example.com/yet.another.thingy'></embed>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_iframe_s_are_added_to_links_if_src_is_set() {
        let input = concat!(
            "<iframe src=\"https://www.youtube.com/embed/0123456789A\"",
            "        width=\"640\" height=\"360\"></iframe>"
        );

        let (lines, links) = render!(input);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[iframe 1 (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(
                    Softwrappable,
                    "[1]: https://www.youtube.com/embed/0123456789A (iframe)"
                ),
            ]
        );

        assert_eq!(
            links,
            [link!("https://www.youtube.com/embed/0123456789A", Iframe)]
        );
    }

    #[test]
    fn t_htmlrenderer_iframe_s_are_rendered_with_a_title_if_title_is_set() {
        let input = concat!(
            "<iframe src=\"https://www.youtube.com/embed/0123456789A\"",
            "        title=\"My Video\" width=\"640\" height=\"360\"></iframe>"
        );

        let (lines, links) = render!(input);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[iframe 1: My Video (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(
                    Softwrappable,
                    "[1]: https://www.youtube.com/embed/0123456789A (iframe)"
                ),
            ]
        );
        assert_eq!(
            links,
            [link!("https://www.youtube.com/embed/0123456789A", Iframe)]
        );
    }

    #[test]
    fn t_htmlrenderer_iframe_s_are_ignored_if_src_is_not_set() {
        let input = "<iframe width=\"640\" height=\"360\"></iframe>";
        let (lines, links) = render!(input);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_spaces_and_line_breaks_are_preserved_inside_pre() {
        let input = concat!(
            "<pre>oh cool\n",
            "\n",
            "  check this\tstuff  out!\n",
            "     \n",
            "neat huh?\n",
            "</pre>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Softwrappable, "oh cool"),
                line!(Softwrappable, ""),
                line!(Softwrappable, "  check this\tstuff  out!"),
                line!(Softwrappable, "     "),
                line!(Softwrappable, "neat huh?"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_tags_still_work_inside_pre() {
        let input = concat!(
            "<pre>",
            "<strong>bold text</strong>",
            "<u>underlined text</u>",
            "</pre>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [line!(Softwrappable, "<b>bold text</><u>underlined text</>")]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_line_breaks_are_preserved_in_tags_inside_pre() {
        let input = concat!(
            "<pre><span>\n\n\n",
            "very cool</span><span>very cool indeed\n\n\n",
            "</span></pre>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Softwrappable, ""),
                line!(Softwrappable, ""),
                line!(Softwrappable, "very coolvery cool indeed"),
                line!(Softwrappable, ""),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_img_results_in_a_placeholder_and_a_link() {
        let input = "<img src='http://example.com/image.png'></img>";

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1 (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/image.png (image)"),
            ]
        );
        assert_eq!(links, [link!("http://example.com/image.png", Img)]);
    }

    #[test]
    fn t_htmlrenderer_img_results_in_a_placeholder_with_the_correct_index_and_a_link() {
        let input = concat!(
            "<a href='http://example.com/index.html'>My Page</a>",
            " and an image: ",
            "<img src='http://example.com/image.png'></img>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(
                    Wrappable,
                    "<u>My Page</>[1] and an image: [image 1 (link #2)]"
                ),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/index.html (link)"),
                line!(Softwrappable, "[2]: http://example.com/image.png (image)"),
            ]
        );

        assert_eq!(
            links,
            [
                link!("http://example.com/index.html", Href),
                link!("http://example.com/image.png", Img),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_img_without_src_or_empty_src_are_ignored() {
        let input = concat!(
            "<img></img>",
            "<img src=''></img>",
            "<img src='http://example.com/image.png'>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1 (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/image.png (image)"),
            ]
        );
        assert_eq!(links.len(), 1);
    }

    #[test]
    fn t_htmlrenderer_alt_is_mentioned_in_placeholder_if_img_has_alt() {
        let input = concat!(
            "<img src='http://example.com/image.png'",
            "alt='Just a test image'></img>",
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1: Just a test image (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/image.png (image)"),
            ]
        );
        assert_eq!(links, [link!("http://example.com/image.png", Img)]);
    }

    #[test]
    fn t_htmlrenderer_alt_is_mentioned_in_placeholder_if_img_has_alt_and_title() {
        let input = concat!(
            "<img src='http://example.com/image.png'",
            "alt='Just a test image' title='Image title'></img>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1: Just a test image (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/image.png (image)"),
            ]
        );
        assert_eq!(links, [link!("http://example.com/image.png", Img)]);
    }

    #[test]
    fn t_htmlrenderer_title_is_mentioned_in_placeholder_if_img_has_title_but_not_alt() {
        let input = concat!(
            "<img src='http://example.com/image.png'",
            "title='Just a test image'></img>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1: Just a test image (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/image.png (image)"),
            ]
        );
        assert_eq!(links, [link!("http://example.com/image.png", Img)]);
    }

    #[test]
    fn t_htmlrenderer_url_of_img_with_data_inside_src_is_replaced_with_string_inline_image() {
        let input = concat!(
            "<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUA",
            "AAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO",
            "9TXL0Y4OHwAAAABJRU5ErkJggg==' alt='Red dot' />"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[image 1: Red dot (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: inline image (image)"),
            ]
        );
        assert_eq!(links, [link!("inline image", Img)]);
    }

    #[test]
    fn t_htmlrenderer_blockquote_is_indented_and_is_separated_by_empty_lines() {
        let input = concat!(
            "<blockquote>",
            "Experience is what you get when you didn't get what you ",
            "wanted. ",
            "&mdash;Randy Pausch",
            "</blockquote>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(lines, [
            line!(Wrappable, ""),
            line!(Wrappable, "  Experience is what you get when you didn't get what you wanted. —Randy Pausch"),
            line!(Wrappable, ""),
        ]);
        assert_eq!(links, []);
    }

    #[test]
    fn dl_dt_and_dd_are_rendered_as_a_set_of_paragraphs_with_term_descriptions_indented_to_the_right(
    ) {
        let input = concat!(
            // Opinions are lifted from the "Monstrous Regiment" by Terry
            // Pratchett
            "<dl>",
            "<dt>Coffee</dt>",
            "<dd>Foul muck</dd>",
            "<dt>Tea</dt>",
            "<dd>Soldier's friend</dd>",
            "</dl>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "Coffee"),
                line!(Wrappable, ""),
                line!(Wrappable, "        Foul muck"),
                line!(Wrappable, ""),
                line!(Wrappable, "Tea"),
                line!(Wrappable, ""),
                line!(Wrappable, "        Soldier's friend"),
                line!(Wrappable, ""),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrendered_h1_is_rendered_with_setext_style_underlining() {
        let input = "<h1>Why are we here?</h1>";
        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "Why are we here?"),
                line!(Wrappable, "----------------"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrendered_when_alone_heading_and_p_tags_generates_only_one_line() {
        for tag in ["<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"] {
            let mut closing_tag = tag.to_string();
            closing_tag.insert(1, '/');

            let input = format!("{}hello world{}", tag, closing_tag);
            let (lines, links) = render!(&input, URL);
            assert_eq!(lines, [line!(Wrappable, "hello world")]);
            assert_eq!(links, []);
        }
    }

    #[test]
    fn t_htmlrenderer_there_s_always_an_empty_line_between_header_paragraph_list_and_paragraph() {
        // h1
        let input = "<h1>header</h1><p>paragraph</p>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(lines[2], line!(Wrappable, ""));
        assert_eq!(links, []);

        // other headings and p
        for tag in ["<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"] {
            let mut closing_tag = tag.to_string();
            closing_tag.insert(1, '/');

            let input = format!("{}header{}<p>paragraph</p>", tag, closing_tag);
            let (lines, links) = render!(&input, URL);
            assert_eq!(lines.len(), 3);
            assert_eq!(lines[1], line!(Wrappable, ""));
            assert_eq!(links, []);
        }

        // ul
        let input = "<ul><li>one</li><li>two</li></ul><p>paragraph</p>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 5);
        assert_eq!(lines[3], line!(Wrappable, ""));
        assert_eq!(links, []);

        // ol
        let input = "<ol><li>one</li><li>two</li></ol><p>paragraph</p>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 5);
        assert_eq!(lines[3], line!(Wrappable, ""));
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_whitespace_is_erased_at_the_beginning_of_the_paragraph() {
        let input = "<p>     \nhere comes the text</p>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines, [line!(Wrappable, "here comes the text")]);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_newlines_are_replaced_with_space() {
        let input = "newlines\nshould\nbe\nreplaced\nwith\na\nspace\ncharacter.";
        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [line!(
                Wrappable,
                "newlines should be replaced with a space character."
            )]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_paragraph_is_just_a_long_line_of_text() {
        let input = concat!(
            "<p>",
            "here comes a long, boring chunk text that we have to fit to ",
            "width",
            "</p>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [line!(
                Wrappable,
                "here comes a long, boring chunk text that we have to fit to width"
            )]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_default_style_for_ol_is_arabic_numerals() {
        // no `type' attribute
        let input = "<ol><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, " 1. one"), line!(Wrappable, " 2. two"),]
        );
        assert_eq!(links, []);

        // invalid `type' attribute
        let input = "<ol type='invalid value'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, " 1. one"), line!(Wrappable, " 2. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_default_starting_number_for_ol_is_1() {
        // no `start` attribute
        let input = "<ol><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, " 1. one"), line!(Wrappable, " 2. two"),]
        );
        assert_eq!(links, []);

        // invalid `start` attribute
        let input = "<ol start='whatever'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, " 1. one"), line!(Wrappable, " 2. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_type_1_for_ol_means_arabic_numbering() {
        let input = "<ol type='1'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, " 1. one"), line!(Wrappable, " 2. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_type_a_for_ol_means_lowercase_alphabetic_numbering() {
        let input = "<ol type='a'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, "a. one"), line!(Wrappable, "b. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_type_uppercase_a_for_ol_means_uppercase_alphabetic_numbering() {
        let input = "<ol type='A'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, "A. one"), line!(Wrappable, "B. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_type_i_for_ol_means_lowercase_roman_numbering() {
        let input = "<ol type='i'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, "i. one"), line!(Wrappable, "ii. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_type_uppercase_i_for_ol_means_uppercase_roman_numbering() {
        let input = "<ol type='I'><li>one</li><li>two</li></ol>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 4);
        assert_eq!(
            lines[1..3],
            [line!(Wrappable, "I. one"), line!(Wrappable, "II. two"),]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_every_next_li_implicitly_closes_the_previous_one() {
        let input = concat!(
            "<ol type='I'>",
            "<li>one",
            "<li>two</li>",
            "<li>three</li>",
            "</li>",
            "<li>four</li>",
            "</ol>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(lines.len(), 6);
        assert_eq!(
            lines[1..5],
            [
                line!(Wrappable, "I. one"),
                line!(Wrappable, "II. two"),
                line!(Wrappable, "III. three"),
                line!(Wrappable, "IV. four"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_style_tags_are_ignored() {
        let input = "<style><h1>ignore me</h1><p>and me</p> body{width: 100%;}</style>";
        let (lines, links) = render!(input, URL);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_hr_is_not_a_string_but_a_special_type_of_line() {
        let input = "<hr>";
        let (lines, links) = render!(input, URL);
        assert_eq!(links, []);
        assert_eq!(lines.len(), 1);
        assert_eq!(lines[0].0, LineType::Hr);
    }

    #[test]
    fn t_htmlrenderer_header_rows_of_tables_are_in_bold() {
        let input = concat!("<table>", "<tr>", "<th>header</th>", "</tr>", "</table>");

        let (lines, links) = render!(input, URL);
        assert_eq!(lines, [line!(Nonwrappable, "<b>header</>"),]);
        assert_eq!(links, []);

        let input = concat!(
            "<table>",
            "<tr>",
            "<th>another</th>",
            "<th>header</th>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(lines, [line!(Nonwrappable, "<b>another</> <b>header</>")]);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_cells_are_separated_by_space_if_border_is_not_set() {
        let input = concat!(
            "<table>",
            "<tr>",
            "<td>hello</td>",
            "<td>world</td>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(lines, [line!(Nonwrappable, "hello world"),]);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_cells_are_separated_by_vertical_bar_if_border_is_set_regardless_of_actual_value(
    ) {
        for border_width in 1..10 {
            let input = format!(
                "<table border='{}'><tr><td>hello</td><td>world</td></tr></table>",
                border_width
            );
            let (lines, links) = render!(&input, URL);
            assert_eq!(lines.len(), 3);
            assert_eq!(lines[1], line!(Nonwrappable, "|hello|world|"));
            assert_eq!(links, []);
        }
    }

    #[test]
    fn t_htmlrenderer_tables_with_border_have_borders() {
        let input = concat!(
            "<table border='1'>",
            "<tr>",
            "<td>hello</td>",
            "<td>world</td>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+-----+-----+"),
                line!(Nonwrappable, "|hello|world|"),
                line!(Nonwrappable, "+-----+-----+"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_if_document_ends_before_table_is_found_table_is_rendered_anyway() {
        let input = concat!(
            "<table border='1'>",
            "<tr>",
            "<td>hello</td>",
            "<td>world</td>",
            "</tr>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+-----+-----+"),
                line!(Nonwrappable, "|hello|world|"),
                line!(Nonwrappable, "+-----+-----+"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_tables_can_be_nested() {
        let input = concat!(
            "<table border='1'>",
            "<tr>",
            "<td>",
            "<table border='1'>",
            "<tr>",
            "<td>hello</td>",
            "<td>world</td>",
            "</tr>",
            "<tr>",
            "<td>another</td>",
            "<td>row</td>",
            "</tr>",
            "</table>",
            "</td>",
            "<td>lonely cell</td>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+---------------+-----------+"),
                line!(Nonwrappable, "|+-------+-----+|lonely cell|"),
                line!(Nonwrappable, "||hello  |world||           |"),
                line!(Nonwrappable, "|+-------+-----+|           |"),
                line!(Nonwrappable, "||another|row  ||           |"),
                line!(Nonwrappable, "|+-------+-----+|           |"),
                line!(Nonwrappable, "+---------------+-----------+"),
            ]
        );

        assert_eq!(links, []);
    }

    #[test]
    fn if_td_appears_inside_table_but_outside_of_a_row_one_is_created_implicitly() {
        let input = concat!(
            "<table border='1'>",
            "<td>hello</td>",
            "<td>world</td>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+-----+-----+"),
                line!(Nonwrappable, "|hello|world|"),
                line!(Nonwrappable, "+-----+-----+"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_previous_row_is_implicitly_closed_when_tr_is_found() {
        let input = concat!(
            "<table border='1'>",
            "<tr><td>hello</td>",
            "<tr><td>world</td>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+-----+"),
                line!(Nonwrappable, "|hello|"),
                line!(Nonwrappable, "+-----+"),
                line!(Nonwrappable, "|world|"),
                line!(Nonwrappable, "+-----+"),
            ]
        );

        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_free_standing_text_outside_of_td_is_implicitly_concatenated_with_the_next_cell(
    ) {
        let input = concat!(
            "<table border='1'>",
            "hello",
            "<td> world</td>",
            "</tr>",
            "</table>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Nonwrappable, "+-----------+"),
                line!(Nonwrappable, "|hello world|"),
                line!(Nonwrappable, "+-----------+"),
            ]
        );

        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_text_within_ituneshack_is_to_be_treated_specially() {
        let input = concat!(
            "<ituneshack>",
            "hello world!\n",
            "I'm a description from an iTunes feed. ",
            "Apple just puts plain text into &lt;summary&gt; tag.",
            "</ituneshack>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "hello world!"),
                line!(Wrappable, "I'm a description from an iTunes feed. Apple just puts plain text into <>summary> tag."),
            ]
        );

        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_when_rendering_text_strips_leading_whitespace() {
        // issue204

        let input = concat!(
            "		<br />\n",
            "		\n",      // tabs
            "       \n", // spaces
            "		\n",      // tabs
            "       \n", // spaces
            "		\n",      // tabs
            "		\n",      // tabs
            "		Text preceded by whitespace."
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, ""),
                line!(Wrappable, "Text preceded by whitespace."),
            ]
        );

        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_video_results_in_a_placeholder_and_a_link_for_each_valid_source() {
        let input = concat!(
            "<video src='http://example.com/video.avi'></video>",
            "<video>",
            "	<source src='http://example.com/video2.avi'>",
            "	<source src='http://example.com/video2.mkv'>",
            "</video>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[video 1 (link #1)]"),
                line!(Wrappable, "[video 2 (link #2)]"),
                line!(Wrappable, "[video 2 (link #3)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/video.avi (video)"),
                line!(Softwrappable, "[2]: http://example.com/video2.avi (video)"),
                line!(Softwrappable, "[3]: http://example.com/video2.mkv (video)"),
            ]
        );

        assert_eq!(
            links,
            [
                link!("http://example.com/video.avi", Video),
                link!("http://example.com/video2.avi", Video),
                link!("http://example.com/video2.mkv", Video),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_video_s_without_valid_sources_are_ignored() {
        let input = concat!(
            "<video></video>",
            "<video><source><source></video>",
            "<video src=''></video>",
            "<video src='http://example.com/video.avi'></video>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[video 1 (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/video.avi (video)"),
            ]
        );
        assert_eq!(links.len(), 1);
    }

    #[test]
    fn t_htmlrenderer_audio_results_in_a_placeholder_and_a_link_for_each_valid_source() {
        let input = concat!(
            "<audio src='http://example.com/audio.oga'></audio>",
            "<audio>",
            "	<source src='http://example.com/audio2.mp3'>",
            "	<source src='http://example.com/audio2.m4a'>",
            "</audio>"
        );

        let (lines, links) = render!(input, URL);
        assert_eq!(
            lines,
            [
                line!(Wrappable, "[audio 1 (link #1)]"),
                line!(Wrappable, "[audio 2 (link #2)]"),
                line!(Wrappable, "[audio 2 (link #3)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/audio.oga (audio)"),
                line!(Softwrappable, "[2]: http://example.com/audio2.mp3 (audio)"),
                line!(Softwrappable, "[3]: http://example.com/audio2.m4a (audio)"),
            ]
        );

        assert_eq!(
            links,
            [
                link!("http://example.com/audio.oga", Audio),
                link!("http://example.com/audio2.mp3", Audio),
                link!("http://example.com/audio2.m4a", Audio),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_audios_without_valid_sources_are_ignored() {
        let input = concat!(
            "<audio></audio>",
            "<audio><source><source></audio>",
            "<audio src=''></audio>",
            "<audio src='http://example.com/audio.oga'></audio>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[audio 1 (link #1)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/audio.oga (audio)"),
            ]
        );
        assert_eq!(links.len(), 1);
    }

    #[test]
    fn t_htmlrenderer_unclosed_video_and_audio_tags_are_closed_upon_encounter_with_a_new_media_element(
    ) {
        let input = concat!(
            "<video src='http://example.com/video.avi'>",
            "	This is fallback text for `the video` element",
            "<video>",
            "	<source src='http://example.com/video2.avi'>",
            "This maybe isn't fallback text, but the spec says that",
            " anything before the closing tag is transparent content",
            "<audio>",
            "	<source src='http://example.com/audio.oga'>",
            "	<source src='http://example.com/audio.m4a'>",
            "	This text should also be interpreted as fallback",
            "<audio src='http://example.com/audio2.mp3'>",
            "	This is additional fallback text",
            "<audio></audio>",
            "Here comes the text!"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[video 1 (link #1)]"),
                line!(Wrappable, "[video 2 (link #2)]"),
                line!(Wrappable, "[audio 1 (link #3)]"),
                line!(Wrappable, "[audio 1 (link #4)]"),
                line!(Wrappable, "[audio 2 (link #5)]"),
                line!(Wrappable, "Here comes the text!"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/video.avi (video)"),
                line!(Softwrappable, "[2]: http://example.com/video2.avi (video)"),
                line!(Softwrappable, "[3]: http://example.com/audio.oga (audio)"),
                line!(Softwrappable, "[4]: http://example.com/audio.m4a (audio)"),
                line!(Softwrappable, "[5]: http://example.com/audio2.mp3 (audio)"),
            ]
        );

        assert_eq!(
            links,
            [
                link!("http://example.com/video.avi", Video),
                link!("http://example.com/video2.avi", Video),
                link!("http://example.com/audio.oga", Audio),
                link!("http://example.com/audio.m4a", Audio),
                link!("http://example.com/audio2.mp3", Audio),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_empty_source_tags_do_not_increase_the_link_count_media_elements_without_valid_sources_do_not_increase_the_element_count(
    ) {
        let input = concat!(
            "<video></video>",
            "<video>",
            "	<source src='http://example.com/video.avi'>",
            "	<source>",
            "	<source src='http://example.com/video.mkv'>",
            "</video>",
            "<audio></audio>",
            "<audio>",
            "	<source src='http://example.com/audio.mp3'>",
            "	<source>",
            "	<source src='http://example.com/audio.oga'>",
            "</audio>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[video 1 (link #1)]"),
                line!(Wrappable, "[video 1 (link #2)]"),
                line!(Wrappable, "[audio 1 (link #3)]"),
                line!(Wrappable, "[audio 1 (link #4)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: http://example.com/video.avi (video)"),
                line!(Softwrappable, "[2]: http://example.com/video.mkv (video)"),
                line!(Softwrappable, "[3]: http://example.com/audio.mp3 (audio)"),
                line!(Softwrappable, "[4]: http://example.com/audio.oga (audio)"),
            ]
        );

        assert_eq!(
            links,
            [
                link!("http://example.com/video.avi", Video),
                link!("http://example.com/video.mkv", Video),
                link!("http://example.com/audio.mp3", Audio),
                link!("http://example.com/audio.oga", Audio),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_back_to_back_video_and_audio_tags_are_seperated_by_a_new_line() {
        let input = concat!(
            "<video src='https://example.com/video.mp4'></video>",
            "<audio src='https://example.com/audio.mp3'></audio>"
        );

        let (lines, links) = render!(input, URL);

        assert_eq!(
            lines,
            [
                line!(Wrappable, "[video 1 (link #1)]"),
                line!(Wrappable, "[audio 1 (link #2)]"),
                line!(Wrappable, ""),
                line!(Wrappable, "Links: "),
                line!(Softwrappable, "[1]: https://example.com/video.mp4 (video)"),
                line!(Softwrappable, "[2]: https://example.com/audio.mp3 (audio)"),
            ]
        );
        assert_eq!(
            links,
            [
                link!("https://example.com/video.mp4", Video),
                link!("https://example.com/audio.mp3", Audio),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_ordered_list_can_contain_unordered_list_in_its_items() {
        let input = concat!(
            "<ol>",
            "	<li>",
            "		<ul>",
            "			<li>first item of sublist A</li>",
            "			<li>second item of sublist A</li>",
            "		</ul>",
            "	</li>",
            "	<li>",
            "		<ul>",
            "			<li>first item of sublist B</li>",
            "			<li>second item of sublist B</li>",
            "			<li>third item of sublist B</li>",
            "		</ul>",
            "	</li>",
            "</ol>"
        );
        let (lines, links) = render!(input);

        assert_eq!(links, []);
        assert_eq!(
            lines,
            [
                line!(Wrappable, ""),
                line!(Wrappable, " 1.  "),
                line!(Wrappable, ""),
                line!(Wrappable, "      * first item of sublist A"),
                line!(Wrappable, "      * second item of sublist A"),
                line!(Wrappable, ""),
                line!(Wrappable, " 2.  "),
                line!(Wrappable, ""),
                line!(Wrappable, "      * first item of sublist B"),
                line!(Wrappable, "      * second item of sublist B"),
                line!(Wrappable, "      * third item of sublist B"),
                line!(Wrappable, ""),
                line!(Wrappable, ""),
            ]
        );
    }

    #[test]
    fn t_htmlrenderer_skips_contents_of_script_tags() {
        // This is a regression test for https://github.com/newsboat/newsboat/issues/1300

        let input = include_str!("../../../test/data/1300-reproducer.html");
        let (lines, links) = render!(input);
        assert_eq!(lines, []);
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_div_is_always_rendered_on_a_new_line() {
        let (lines, links) = render!(concat!(
            "<div>oh</div>",
            "<div>hello there,</div>",
            "<p>world</p>",
            "<div>!</div>",
            "<div>",
            "    <div>",
            "        <p>hehe</p>",
            "    </div>",
            "</div>"
        ));

        assert_eq!(
            lines,
            [
                line!(Wrappable, "oh"),
                line!(Wrappable, ""),
                line!(Wrappable, "hello there,"),
                line!(Wrappable, ""),
                line!(Wrappable, "world"),
                line!(Wrappable, ""),
                line!(Wrappable, "!"),
                line!(Wrappable, ""),
                line!(Wrappable, "hehe"),
            ]
        );
        assert_eq!(links, []);
    }

    #[test]
    fn t_htmlrenderer_does_not_crash_on_extra_closing_ol_ul_tags() {
        // This is a regression test for https://github.com/newsboat/newsboat/issues/1974
        render!("<ul><li>Double closed list</li></ul></ul><ol><li>Other double closed list</li></ol></ol><ul><li>Test</li></ul>");
    }
}
