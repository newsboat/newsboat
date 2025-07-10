#!/bin/sh
# create a getpocket.com authentication token and store it in
# ~/.pocket_access_token. As this is basic oauth2 stuff maybe
# other sites should work in a similar fashion
#
# (c) 2013, Andreas Happe <andreashappe@snikt.net>

APPLICATION_CONSUMER_KEY="19002-18d9a9028a5ae783a9caabcd"

output=`wget --post-data '{ "consumer_key":"19002-18d9a9028a5ae783a9caabcd", "redirect_uri":"https://github.com/Newsboat/Newsboat/blob/c8c92a17fa0862fb7a648e88723eb48cb9cb582c/contrib/getpocket.com/after_authentication.md"}' --header="Content-Type: application/json; charset=UTF-8" --header "X-Accept: application/json" "https://getpocket.com/v3/oauth/request" -O - 2>/dev/null`

# shamelessy copy this from contrib/bookmark-pinboard
TMP_TOKEN=`echo $output | sed 's/^.*\"code\":"\([^"]*\)".*$/\1/'`

# redirect user to pocket authentication page
AUTH_URL="https://getpocket.com/auth/authorize?request_token=$TMP_TOKEN&redirect_uri=https://github.com/Newsboat/Newsboat/blob/c8c92a17fa0862fb7a648e88723eb48cb9cb582c/contrib/getpocket.com/after_authentication.md"
echo $TMP_TOKEN> /tmp/pocket_token
echo "please navigate to $AUTH_URL, active the access. Then press enter"

xdg-open $AUTH_URL

read dontcare

output=`wget --post-data "consumer_key=$APPLICATION_CONSUMER_KEY&code=$TMP_TOKEN" https://getpocket.com/v3/oauth/authorize -O - 2>/dev/null`

echo $output > /tmp/input

output=`echo $output | sed 's/^.*access_token=\([^&"]*\).*$/\1/'`

echo $output > ~/.pocket_access_token
