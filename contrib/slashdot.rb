#!/usr/bin/ruby
#
# get, parse and enrich slashdot rss feed
#
# Change history
#
#  26.06.2009    erb    stopped error from appearing if servers do not respond
#
$:.push(File.dirname($0))
require 'feedgrabber'

require 'rexml/document'
include REXML

require 'hpricot'

if ARGV.length > 0
  print "usage: #{File::basename($0)}\n"
  exit
end

if ENV['http_proxy'].nil? && !ENV['HTTP_PROXY'].nil?
  ENV['http_proxy'] = ENV['HTTP_PROXY']
end

feedurl = 'http://rss.slashdot.org/Slashdot/slashdot'

fg = FeedGrabber.new("slashdot")

feed_text = fg.getURL_uncached(feedurl)

exit 3 unless feed_text && feed_text.length >= 20

xml = Document.new(feed_text)

xml.elements.each("//item") do |item|
  # correct entities in title
  title=item.elements['title'].text
  title.gsub!("\&amp;", "\&")
  title.gsub!("\&amp;", "\&") # needs to be in there twice, because the error is in there twice :-)
  title.gsub!("\&lsquo;", "\"")
  title.gsub!("\&rsquo;", "\"")
  title.gsub!("\&quo;", "\"")
  title.gsub!("\&mdash;", "--")
  item.elements['title'].text= title

  # extract link to article
  article_url = item.attributes.get_attribute('rdf:about').value
  article_url.sub!(%r{\?from=rss$}, "/")

  # get full text for article
  begin
    article = fg.getURL(article_url)
  rescue
    next
  end

  next unless article && article.length >= 20

  # now parse the article
  article_text=""
  begin
    article_xml = Hpricot(article)
  rescue
    next
  end

  # /. special: extract the two divisions:
  #   first <div id="text-nnnnnnnn">
  #   first <div class="bodytext">
  # and combine them in that order
  article_xml.search("//div[@id]").each do |divitem|
    if divitem.attributes['id'][0,5] == "text-"
      article_text = "<div>"
      article_text << divitem.inner_html << "</div>"
      break
    end
  end
  article_text << "<div>" << article_xml.search("//div[@class='bodytext']").inner_html << "</div>"

  # get rid of comments and other annoying artifacts
  article_text.gsub!(/<!--[^>]*-->/, "")
  article_text.gsub!(/\s+/m, " ")

  next if article_text.length < 10

  # set full text article into feed
  item.elements['description'].text= CData.new(article_text)
end

fg.cleanupDB

xml.write($stdout, -1)
