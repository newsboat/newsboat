#!/bin/bash
# send an url to getpocket.com, using the token from ~/.pocket_access_token.
#
# (c) 2013, Andreas Happe <andreashappe@snikt.net>

if [ ! -r ~/.pocket_access_token ]; then
  echo "~/.pocket_access_token not found, please call create-pocket-user-token.sh before"
  exit 1
fi

APPLICATION_CONSUMER_KEY="19002-18d9a9028a5ae783a9caabcd"
USER_ACCESS_TOKEN=`cat ~/.pocket_access_token`
METHOD_URL="https://getpocket.com/v3/add"

TITLE="$2"
URL="$1"

PARAMS="{\"url\":\"$URL\", \"title\":\"$TITLE\", \"consumer_key\":\"$APPLICATION_CONSUMER_KEY\", \"access_token\":\"$USER_ACCESS_TOKEN\"}"

output=`wget --post-data "$PARAMS" --header="Content-Type: application/json" $METHOD_URL -O - 2>/dev/null`
