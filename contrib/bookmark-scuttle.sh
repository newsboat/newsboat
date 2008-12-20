#!/bin/sh
# newsbeuter generic bookmarking plugin for a scuttle installation (http://sourceforge.net/projects/scuttle/)
# adapted by Igorette
# (c) 2007 Andreas Krennmair
# documentation: http://delicious.com/help/api#posts_add
# (scuttle is nearly 100% api compatible with delicious.com)

username="your scuttle username here"
password="your scuttle password here"
site="your scuttle installation base url"

url="$1"
title="$2"
desc="$3"

# scuttle installation without url rewriting
scuttle_url=${site}"/api/posts_add.php?url=${url}&description=${title}&extended=${desc}&tag=mytag"

# for clean urls (using mod_rewrite)
# scuttle_url=${site}"/api/posts/add?url=${url}&description=${title}&extended=${desc}&tag=mytag"

output=`wget --http-user=$username --http-passwd=$password -O - "$scuttle_url" 2> /dev/null`

output=`echo $output | sed 's/^.*code="\([^"]*\)".*$/\1/'`

if [ "$output" != "done" ] ; then
  echo "$output"
fi
