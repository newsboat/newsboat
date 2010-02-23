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

#          key            feed URL
FEEDS = { "news"      => "http://www.heise.de/newsticker/heise-atom.xml",
          "telepolis" => "http://www.heise.de/tp/news-atom.xml",
	  "security"  => "http://www.heise.de/security/news/news-atom.xml",
	  "netze"     => "http://www.heise.de/netze/rss/netze-atom.xml",
	  "it-blog"   => "http://www.heise.de/developer/rss/world-of-it/blog-atom.xml"
	}

GOOGLEON="<!--googleon: index-->"
GOOGLEOFF="<!--googleoff: index-->"

def listFeeds
  FEEDS.each_key { |k| print "  #{k}\n" }
end

if ARGV.length < 1
  print "usage: #{File::basename($0)} <feed>\n"
  print "<feed> is one of\n"
  listFeeds
  exit
end

def shortenArticle(article_text)
  article_text.gsub!(/<!\[CDATA\[/, "")
  article_text.gsub!(/\]\]>/, "")

  # now, heise speciality: get everything between GOOGLEON and GOOGLEOFF patterns :-)
  p1 = article_text.index(GOOGLEON)
  p2 = article_text.index(GOOGLEOFF)
  if (p1 && p2)
    result = ""
    pos = p1
    while(pos < article_text.length) do
      p1 = article_text.index(GOOGLEON, pos)
      break unless p1
      p2 = article_text.index(GOOGLEOFF, pos)
      p2 = article_text.length unless p2
      if p1 < p2
        result += article_text[p1+GOOGLEON.length..p2-1]
        pos = p2+GOOGLEOFF.length
      else
        pos = p1+GOOGLEON.length
      end
    end
    article_text = result
  end

  # get rid of comments and other annoying artifacts
  article_text.gsub!(/<!--LINK_ICON--><img[^>]*><!--\/LINK_ICON-->/m, " ")
  article_text.gsub!(/<!--[^>]*-->/, "")
  article_text.gsub!(/\s+/m, " ")
  article_text.gsub!(/href=\"\//m, "href=\"http://www.heise.de/")
  article_text.gsub!(/src=\"\//m, "src=\"http://www.heise.de/")

  article_text
end

feed=ARGV[0]

unless FEEDS.has_key?(feed)
  print "unknown feed '#{feed}'. Use one of these:\n"
  listFeeds
  exit
end

feedurl = FEEDS[feed]

#get feed
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
xml.elements.each("//entry") do |item|

  # extract link to article
  article_url = item.elements['id'].text
  article_url.sub!(%r{from/.*$}, "")
  article_short_url = article_url.sub(%r{/[^/]*--/}, "/")

  # get full text for article
  article_text = ""
  retries = 4
  begin
#    print "<!-- Reading article from ", article_url, " -->\n"; STDOUT.flush
    Timeout::timeout(15) do
      article = open(article_url)
      article_text = article.read unless article.nil?
    end
  rescue Timeout::Error
    retries -= 1
    next if retries < 1
    sleep 1
    retry
  rescue
  end

  next if article_text.length < 20

  article_text = shortenArticle(article_text)

  # insert full text article into feed
  description = Element.new("content")
  description.text= CData.new(article_text)
  item.add_element(description)

  #guid = Element.new("guid")
  #guid.text= article_short_url
  #item.add_element(guid)
end
  
#reproduce enriched feed
xml.write($stdout, -1)
