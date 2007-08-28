#!/bin/sh
# newsbeuter bookmarking plugin for del.icio.us
# (c) 2007 Andreas Krennmair
# documentation: http://del.icio.us/help/api/posts

username="your del.icio.us username here"
password="your del.icio.us password here"

url="$1"
title="$2"
desc="$3"

delicious_url="https://api.del.icio.us/v1/posts/add?url=${url}&description=${title}&extended=${desc}"

output=`wget --http-user=$username --http-passwd=$password -O - "$delicious_url" 2> /dev/null`

output=`echo $output | sed 's/^.*code="\([^"]*\)".*$/\1/'`

if [ "$output" != "done" ] ; then
  echo "$output"
fi
