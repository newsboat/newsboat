#!/usr/bin/ruby
#
# This module has the methods to get, parse and enrich rss feeds
#
# Change history
#
#  29.03.2010    erb    classes and methods to ease up feed parser writing
#
require 'net/http'
require "uri"
require 'timeout'
require 'gdbm'

#
# This class capsules DB caching and HTTP getting pages for articles
#
# You instantiate a FeedGrabber giving at least a unique feed name (for DB
# name generation) Then you can call getURL with the article url to get.  If
# it is found in the cache, that one is retrieved.  If you parsed your feed
# you should call cleanupDB to get rid of all those unused entries in the
# cache
#
#module FeedGrabber
  class FeedGrabber
    # create a FeedGrabber instance
    def initialize(uniqueName, path = nil, retries = 4, depth = 5, timeout = 15)
      path = File.expand_path("~") if path.nil?
      @dbCacheName = "#{path}/.Newsboat/#{uniqueName}.db"       # generate db cache filename
      @maxRetries = retries
      @maxDepth = depth
      @timeout = timeout

      @usedURLs = Array.new                               # empty array to hold used URLs
    end

    #
    # try to retrieve web site, following up to maxDepth redirects, having up to maxRetries retries
    #
    def getURL_uncached(url)
      result = nil
      retries = @maxRetries
      begin
        Timeout::timeout(@timeout) do
          tempurl = url
          depth = @maxRetries
          while true
            raise ArgumentError, "Followed more #{@maxDepth} redirections. Stopping this nightmare now." if depth == 0
            response = Net::HTTP.get_response(URI.parse(tempurl))
            case response
              when Net::HTTPSuccess     then
                result = response.body
                break
              when Net::HTTPRedirection then
                tempurl = response['location']
                depth -= 1
                next # follow redirection
            end
          end
        end
      rescue Timeout::Error
        retries -= 1
        exit 1 if retries < 1
        sleep 1
        retry
      rescue # maybe an ArgumentError or anything the net layer throws
        # any other error shall not make any noise (maybe shall we produce a fake RSS item)
      end
      result
    end

    #
    # get url, but create and use a DB cache for each feed
    #
    def getURL(url)
      @usedURLs << url    # remember, we used that URL for cleanup later
      db = GDBM.new(@dbCacheName)
      if db.has_key?(url)
        data = db[url]      # get cached data from DB
      else
        # not in DB? so get it and store it into DB
        data = getURL_uncached(url)
        db[url] = data
      end
      db.close
      data
    end

    #
    # remove all URLs not used from DB cache
    #
    def cleanupDB
      toRemove = Array.new
      db = GDBM.new(@dbCacheName)
      db.each_key do |key|
        toRemove << key if @usedURLs.index(key) == nil
      end
      toRemove.each do |url|
        db.delete(url)
      end
      db.close
    end
  end #class
#end # module
