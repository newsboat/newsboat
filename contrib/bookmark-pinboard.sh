#!/bin/sh
# newsbeuter bookmarking plugin for pinboard
# (c) 2007 Andreas Krennmair
# documentation: https://pinboard.in/api

username="pinboard_username"
password="pinboard_password"

tag="via:newsbeuter"

url="$1"
title="$2"
desc="$3"

pinboard_url="https://api.pinboard.in/v1/posts/add?url=${url}&description=${title}&extended=${desc}&tags="${tag}""

output=`wget --http-user=$username --http-passwd=$password -O - "$pinboard_url" 2> /dev/null`

output=`echo $output | sed 's/^.*code="\([^"]*\)".*$/\1/'`

if [ "$output" != "done" ] ; then
  echo "$output"
fi
