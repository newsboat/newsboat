#!/usr/bin/env ruby

require 'httparty' # gem install httparty
require 'json'

# Set this to your own instance's API URL:
linkding_uri = "http://example.linkding.com/api/bookmarks"

# Get your Linkding API key from Settings > Integrations
# Set the LINKDING_TOKEN env variable with you Linkding key, or just hardcode it here.
token = ENV['LINKDING_TOKEN']

link_url = ARGV[0]
link_title = ARGV[1]
description = ARGV[2]
website_title = ARGV[3]

params = {url: URI(link_url), title: link_title, website_title: website_title, unread: true}
headers = {'Content-Type' => "application/json", 'Authorization' => "Token #{token}"}

resp = HTTParty.post(linkding_uri, body: params.to_json, headers: headers)
