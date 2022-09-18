use libnewsboat::htmlrenderer;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct HtmlRenderer(htmlrenderer::HtmlRenderer);

#[cxx::bridge(namespace = "newsboat::htmlrenderer::bridged")]
mod bridged {
    // This has to be in sync with logger::Level in rust/libnewsboat/src/htmlrenderer.rs

    enum LineType {
        WRAPPABLE = 1,
        SOFTWRAPPABLE,
        NONWRAPPABLE,
        HR,
    }

    enum LinkType {
        HREF,
        IMG,
        EMBED,
        IFRAME,
        VIDEO,
        AUDIO,
    }

    struct Line {
        line_type: LineType,
        content: String,
    }

    struct Link {
        link_type: LinkType,
        content: String,
    }

    extern "Rust" {
        type HtmlRenderer;

        fn create(raw: bool) -> Box<HtmlRenderer>;
        fn render(
            obj: &mut HtmlRenderer,
            source: &str,
            line_types: &mut Vec<LineType>,
            lines: &mut Vec<String>,
            links_types: &mut Vec<LinkType>,
            links: &mut Vec<String>,
            url: &str,
        );
        fn render_hr(width: u32) -> String;
    }
}

fn create(raw: bool) -> Box<HtmlRenderer> {
    Box::new(HtmlRenderer(htmlrenderer::HtmlRenderer::new(raw)))
}

fn render_hr(width: u32) -> String {
    htmlrenderer::HtmlRenderer::render_hr(width)
}

fn render(
    obj: &mut HtmlRenderer,
    source: &str,
    line_types: &mut Vec<bridged::LineType>,
    lines: &mut Vec<String>,
    link_types: &mut Vec<bridged::LinkType>,
    links: &mut Vec<String>,
    url: &str,
) {
    let mut line_pairs = Vec::new();
    let mut link_pairs = Vec::new();
    obj.0.render(source, &mut line_pairs, &mut link_pairs, url);
    for (ty, content) in line_pairs {
        line_types.push(line_ty(ty));
        lines.push(content);
    }
    for (content, ty) in link_pairs {
        link_types.push(link_ty(ty));
        links.push(content);
    }
}

fn link_ty(ty: htmlrenderer::LinkType) -> bridged::LinkType {
    match ty {
        htmlrenderer::LinkType::Href => bridged::LinkType::HREF,
        htmlrenderer::LinkType::Img => bridged::LinkType::IMG,
        htmlrenderer::LinkType::Embed => bridged::LinkType::EMBED,
        htmlrenderer::LinkType::Iframe => bridged::LinkType::IFRAME,
        htmlrenderer::LinkType::Video => bridged::LinkType::VIDEO,
        htmlrenderer::LinkType::Audio => bridged::LinkType::AUDIO,
    }
}

fn line_ty(ty: htmlrenderer::LineType) -> bridged::LineType {
    match ty {
        htmlrenderer::LineType::Wrappable => bridged::LineType::WRAPPABLE,
        htmlrenderer::LineType::Softwrappable => bridged::LineType::SOFTWRAPPABLE,
        htmlrenderer::LineType::Nonwrappable => bridged::LineType::NONWRAPPABLE,
        htmlrenderer::LineType::Hr => bridged::LineType::HR,
    }
}
