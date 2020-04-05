/// Types of links that HtmlRenderer collects while rendering
#[derive(Debug, Copy, Clone)]
pub enum LinkType {
    /// Ordinary HTML `<a href="...">...</a>` link
    Href,

    /// HTML `<img>`
    Img,

    /// HTML `<embed>`
    Embed,

    /// HTML `<video>`
    Video,

    /// HTML `<audio>`
    Audio,
    // This enum has to be kept in sync with enum LinkType in include/htmlrenderer.h
}
