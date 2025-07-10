#!/bin/bash

cp urls urls-fltr-substack.old
cat urls | sed 's|\(.*.substack.com/feed\)|filter:~/.config/Newsboat/fltr-substack.sh:\0|g' > new_urls
mv new_urls urls

