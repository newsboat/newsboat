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
# Successfully tested with Asciidoctor 1.5.5 and 2.0.12.

# The following import is required for the old Asciidoctor version 1.x shipped
# in Ubuntu 18.04, which we use in our CI/CD. Asciidoctor 2.x doesn't need it,
# but it also doesn't hurt.
require 'asciidoctor/converter/manpage'

class ManPageConverter < Asciidoctor::Converter::ManPageConverter

  def convert_inline_monospaced node
    %[#{ESC_BS}fI<BOUNDARY>#{node.text}</BOUNDARY>#{ESC_BS}fP]
  end

  # In Asciidoctor version 1.x the method didn't have a 'convert' prefix.
  def inline_quoted node
    case node.type
    when :monospaced
      convert_inline_monospaced node
    else
      # Because of this super call, we have to define the exact same method
      # twice and cannot use an alias. Otherwise the original method name will
      # not be found.
      super
    end
  end

  # In Asciidoctor version 2.x the method got a 'convert' prefix.
  def convert_inline_quoted node
    case node.type
    when :monospaced
      convert_inline_monospaced node
    else
      # Because of this super call, we have to define the exact same method
      # twice and cannot use an alias. Otherwise the original method name will
      # not be found.
      super
    end
  end

end

if defined? Asciidoctor::Converter::Factory.register
  # Registering the converter in Asciidoctor version 1.x must explicitly use
  # the 'Factory'.
  Asciidoctor::Converter::Factory.register ManPageConverter, ['manpage']
else
  # Registering the converter in Asciidoctor version 2.x must not use the
  # 'Factory'. Once support for 1.x is dropped, this can be even simplified to
  # `register_for 'manpage'` in the class definition itself.
  Asciidoctor::Converter.register ManPageConverter, 'manpage'
end

