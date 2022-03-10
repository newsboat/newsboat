# The original Asciidoctor man page converter produces a monospace font
# ('\f(CR') for inline code fragments. The introducing merge request
#
#     https://github.com/asciidoctor/asciidoctor/pull/3561
#
# explains that this helps further transformation of the resulting man pages to
# post script documents or PDF files. With that the inline code is then
# rendered in a nice monospace font in PS or PDF files. This comes to a price,
# though. Traditionally, terminals use monospace fonts exclusively. So when the
# man page is viewed in the terminal, the dedicated monospace font of the
# inlined code does not stand out at all. It just looks like regular text,
# which makes reading it quite hard sometimes.
#
# The Newsboat and Podboat man pages are probably most often viewed in the
# terminal. Transforming them to another document format presumably happens
# only very rarely. Also, a dedicated HTML documentation is offered that covers
# even more topics than the man pages. This fact is reassuring the conjecture
# about man pages being read in the terminal most of the time.
#
# In order to increase the man page readability for terminal users, this custom
# man page converter renderes inline code as underlined text instead.
#
# The downside, however, is, that converting the man page to post script or PDF
# loses the original monospace font. In post script and PDF the inline code is
# then rendered in italics. This appearance is far from the original
# Asciidoctor source. Oh well. If one really wanted to have a nice post script
# document from the man page, just don't use this converter to generate the man
# page.
#
# The official documentation covers the process of creating a custom converter
# excellently: https://docs.asciidoctor.org/asciidoctor/latest/convert/custom/
#
# SuccessfullyTested with Asciidoctor 2.0.12.
class ManPageConverter < (Asciidoctor::Converter.for 'manpage')
  register_for 'manpage'

  def convert_inline_quoted node
    case node.type
    when :monospaced
      %[#{ESC_BS}fI<BOUNDARY>#{node.text}</BOUNDARY>#{ESC_BS}fP]
    else
      super
    end
  end
end

