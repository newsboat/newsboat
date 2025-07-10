#!/usr/bin/env bash
# Newsboat bookmarking plugin for buku
# (c) 2021 Greg Fitzgerald <gregf@beastie.tech>
#
# Heavily inspired by bookmark-pinboard.sh
#
# Add the following to your Newsboat config, and adjust the path as needed.
# bookmark-cmd ~/bin/bookmark-buku.sh
# bookmark-interactive yes


url="$1"
title="$2" #you can comment this out and rely on buku to get the title if you prefer.
#desc="$3" # not used by buku, buku will get this information for you.
#feed_title="$4" don't think this is of any use to us either?

buku=$(command -v buku)

echo "Enter some tags comma seperated: "
read tags

if [ ! "$tags" ]; then
  # You can set default tags here
  tags="Newsboat"
fi

if [ "$buku" ]; then
  buku --nostdin -a "$url" "$tags" --title "$title" >/dev/null
fi
