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
#  28.03.2010    erb    Added DB cache to speed things up (significantly!)
#
$:.push(File.dirname($0))
require 'feedgrabber'

require 'rexml/document'
include REXML

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
fg = FeedGrabber.new("heisecache-#{feed}")
feed_text = fg.getURL_uncached(feedurl)

exit 2 unless feed_text && feed_text.length > 20

xml = Document.new(feed_text)

#loop over items
xml.elements.each("//entry") do |item|
  # extract link to article
  article_url = item.elements['id'].text
  article_url.sub!(%r{from/.*$}, "")
  article_short_url = article_url.sub(%r{/[^/]*--/}, "/")

  # get full text for article
  article_text = fg.getURL(article_url)
  next unless article_text && article_text.length > 20

  # extract article comment link
  begin
    comments = /<a href=\"(\/[a-z\/]*\/foren\/[^\/]*\/forum-[0-9]*\/list\/)\"/m.match(article_text)[1]
  rescue
    comments =""
  end
  article_text = shortenArticle(article_text)

  article_text += "<p><a href=\"http://www.heise.de#{comments}\">Kommentare</a></p>" if comments.length > 5 && comments.length < 150

  # insert full text article into feed
  description = Element.new("content")
  description.add_attribute("type", "html")
  description.text= CData.new(article_text)
  item.add_element(description)
end

fg.cleanupDB

# reproduce the content enriched feed
xml.write($stdout, -1)
