#!/usr/bin/env ruby

require 'httparty' # gem install httparty
require 'json'

entry_url = ARGV[0]

# Change to reflext your own Wallabag instance's base URL
wallabag_url="https://wallabag.example.net"


# Get your client_id and client_secret from Wallabag:
# Settings -> API clients management
# Either hardcode these values or put them into environment variables
client_id = ENV['WALLABAG_CLIENT']
client_secret = ENV['WALLABAG_SECRET']
username = ENV['WALLABAG_USER']
password = ENV['WALLABAG_PASSWORD']

# Get an oauth token. Expires every 30 minutes
token_params = {grant_type: "password", client_id: client_id, client_secret: client_secret, username: username, password: password}
token_req = HTTParty.post("#{wallabag_url}/oauth/v2/token", body: token_params)
access_token = token_req["access_token"]

# Store the bookmark. Wallabag extracts from the page, so we're not sending title, desc, etc.
headers = {'Content-Type' => "application/json", 'Authorization' => "Bearer #{access_token}"}
params = {url: entry_url, starred: 0, archive: 0}
resp = HTTParty.post("#{wallabag_url}/api/entries.json", body: params.to_json, headers: headers)
