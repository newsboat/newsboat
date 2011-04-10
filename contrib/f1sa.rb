#!/usr/bin/ruby
#
# get, parse and enrich heise rss feeds
#
# call with the feed specified you like to retrieve. Currently supported:
#
#  news      - heise newsticker
#  teleopils - Telepolis
#  security  - heise security news
#
# Change history
#
#  26.06.2009    erb    suppressed error messages due to unrepsonsive servers
#

require 'net/http'
require 'uri'

require 'rexml/document'
include REXML

require 'hpricot'

require "open-uri"
require 'timeout'

#try to retrieve web site, following up to 5 redirects
def geturl(url, depth=5)
  raise ArgumentError, 'Followed more 4 redirections. Stopping this nightmare now.' if depth == 0
  response = Net::HTTP.get_response(URI.parse(url))
  case response
    when Net::HTTPSuccess     then response.body
    when Net::HTTPRedirection then geturl(response['location'], depth-1) # follow redirection
  else
    # any other error shall not make any noise (maybe shall we produce a fake RSS item)
    ""
  end
end

if ENV['http_proxy'].nil? && !ENV['HTTP_PROXY'].nil?
  ENV['http_proxy'] = ENV['HTTP_PROXY']
end

feedurl="http://www.f1sa.com/index2.php?option=com_rss&feed=RSS2.0&no_html=1"

# get feed
feed_text = ""
retries=4
begin
  Timeout::timeout(15) do
    f = open(feedurl)
    feed_text = f.read unless f.nil?
  end
rescue Timeout::Error
  retries -= 1
  exit 1 if retries < 1
  sleep 1
  retry
rescue
  # any other error shall not make any noise (maybe shall we produce a fake RSS item)
end

exit 2 if feed_text.length < 20

#print "Got this feed: ", feed_text, "\n"; STDOUT.flush

xml = Document.new(feed_text)

#loop over items
xml.elements.each("//item") do |item|
  # extract link to article
  article_url = item.elements['link'].text

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

  #puts "Got article from #{article_url}"

  # F1SA special: extract the division:
  #   first <div id="body_outer">
  # and combine them in that order
  article_xml.search("//div[@id]").each do |divitem|
    if divitem.attributes['id'] == "body_outer"
      article_text = "<div>"
      article_text << divitem.inner_html << "</div>"
      break
    end
  end

  article_text.gsub!(/<!-- START of joscomment -->.*\Z/m, "")

  #puts "Got this text: #{article_text}"

  # get rid of comments and other annoying artifacts
  article_text.gsub!(/<!--[^>]*-->/, "")
  article_text.gsub!(/\s+/m, " ")

  next if article_text.length < 10

  # insert full text article into feed
  item.delete_element("description")
  description = Element.new("description")
  description.text= CData.new(article_text)
  item.add_element(description)

  guid = Element.new("guid")
  guid.text= article_url
  item.add_element(guid)
end
  
#reproduce enriched feed
xml.write($stdout, -1)
