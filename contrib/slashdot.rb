#!/usr/bin/ruby
#
# get, parse and enrich slashdot rss feed
#
# Change history
#
#  26.06.2009    erb    stopped error from appearing if servers do not respond
#

require 'net/http'
require 'uri'
require 'open-uri'
require 'timeout'

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

feed_text = ""
retries = 4
begin
  Timeout::timeout(15) do
    feed = open(feedurl)
    exit 1 if feed.nil?
    feed_text = feed.read
  end
rescue Timeout::Error
  retries -= 1
  exit 2 if retries < 1
  sleep 1
  retry
end

exit 3 if feed_text.length < 20

xml = Document.new(feed_text)

xml.elements.each("//item") do |item|
  # extract link to article
  article_url = item.attributes.get_attribute('rdf:about').value
  article_url.sub!(%r{\?from=rss$}, "/")

  # get full text for article
  begin
    article = open(article_url)
  rescue
    next
  end
  next if article.nil?

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

xml.write($stdout, -1)
