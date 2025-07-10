#!/bin/bash
#basic Newsboat bookmark plugin for evernote using geeknote: https://github.com/pipakin/geeknote

url="$1"
title="$2"
description="$3"
feed_title="$4"

content="${url}"$'\n'"${description}"$'\n'"${feed_title}"

geeknote create --title "${title}" --content "${content}"
