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

require 'net/http'
require 'uri'

require 'rexml/document'
include REXML

#try to retrieve web site, following up to 5 redirects
def geturl(url, depth=5)
  raise ArgumentError, 'Followed more 4 redirections. Stopping this nightmare now.' if depth == 0
  response = Net::HTTP.get_response(URI.parse(url))
  case response
    when Net::HTTPSuccess     then response.body
    when Net::HTTPRedirection then geturl(response['location'], depth-1) # follow redirection
  else
    response.error!
  end
end

#          key            feed URL
FEEDS = { "news"      => "http://www.heise.de/newsticker/heise.rdf",
          "telepolis" => "http://www.heise.de/tp/news.rdf",
	  "security"  => "http://www.heise.de/security/news/news.rdf"
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

feed=ARGV[0]

unless FEEDS.has_key?(feed)
  print "unknown feed '#{feed}'. Use one of these:\n"
  listFeeds
  exit
end

feedurl = FEEDS[feed]

#get feed
xml = Document.new(geturl(feedurl))

#loop over items
xml.elements.each("//item") { |item|
  # extract link to article
  article_url = item.elements['link'].text
  article_url.sub!(%r{from/rss.*$}, "")
  article_short_url = article_url.sub(%r{/[^/]*--/}, "/")

  # get full text for article
  article_text = geturl(article_url)
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
        pos=p2+GOOGLEOFF.length
      else
        pos=p1+GOOGLEON.length
      end
    end
    article_text = result
  end

  # get rid of comments and other annoying artifacts
  article_text.gsub!(/<!--LINK_ICON--><img[^>]*><!--\/LINK_ICON-->/m, " ")
  article_text.gsub!(/<!--[^>]*-->/, "")
  article_text.gsub!(/\s+/m, " ")

  # insert full text article into feed
  description = Element.new("description")
  description.text= CData.new(article_text)
  item.add_element(description)

  guid = Element.new("guid")
  guid.text= article_short_url
  item.add_element(guid)
}

#reproduce enriched feed
xml.write($stdout, -1)
