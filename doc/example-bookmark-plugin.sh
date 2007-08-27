#!/bin/sh
# this is a simple example script that demonstrates how bookmarking plugins for newsbeuter are implemented
# (c) 2007 Andreas Krennmair

url="$1"
title="$2"
description="$3"

echo -e "${url}\t${title}\t${description}" >> ~/bookmarks.txt
