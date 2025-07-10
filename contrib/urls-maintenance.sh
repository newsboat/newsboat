#!/bin/sh
# operates various maintenance tasks on urls and cache.db for Newsboat:
# - first pass: convert http to https in urls file when https feed is available
# - second pass: convert feeds urls to their new forwarded urls for 3xx http codes
# feeds urls not returning xml content are tagged "_fail"
# cache.db is also updated to refer to the new url feeds so all read/unread articles and flags are preserved
# urls file and cache.db are automatically backed up with timestamp before proceeding

# TODO
# address remaining feedback from https://github.com/Newsboat/Newsboat/pull/647
# implement additional checks on active feeds:
# 	https://www.linuxjournal.com/content/parsing-rss-news-feed-bash-script
#	is it returning valid rss?
#	when was the feed last updated?
#	sort valid feeds by last updated
#	tag feed "abandoned" when most recent pubdate is more 1 year old


#Newsboat urls file and cache locations
u="$HOME/.config/Newsboat/urls"
db="$HOME/.config/Newsboat/cache.db"
#curl timeout for URLs probing
timeout=20
tagfail="_fail"
useragent="Lynx/2.8.5rel.1 libwww-FM/2.14"
#where to dump headers
rss="/tmp/Newsboat-rss.tmp"
headers="/tmp/Newsboat-headers.tmp"

# shuf (GNU coreutils) randomises the urls list, this avoids querying the same domains too fast, assuming urls are grouped by domains or alphasorted in the urls file
requirements="Newsboat curl sqlite3 sed grep awk head shuf"
for app in $requirements
do
	command -v "$app" >/dev/null 2>&1 || { echo >&2 "$app is required but it's not installed or it's not in your PATH. Aborting."; exit 1; }
done

if [ ! -f "$u" ]; then
	echo "$u not found. edit the path/filename for urls file"; exit
fi
if [ ! -f "$db" ]; then
	echo "$db not found. edit the path/filename for cache.db"; exit
fi
if [ -f "$db.lock" ]; then
	echo "Newsboat is still running. Stop it first then try again"; exit
fi

cp "$db" "$db.bak-$(date +%FT%T)"
cp "$u" "$u.bak-$(date +%FT%T)"

_replace () {
	response=$(curl -A "$useragent" --connect-timeout "$timeout" --max-time "$timeout" --write-out "%{http_code}" --silent -D "$headers" --output "$rss" "$url2")
	if [ "$response" = "200" ]; then
		if grep -qiE "content-type: .*xml" "$headers"; then
			#escape any & found in url, this is a special character in sed
			url2=$( echo "$url2" | sed -e 's/[&\\/]/\\&/g' )
			sed -i "s,^$url,$url2," "$u"
			sqlite3 "$db" "update rss_feed set rssurl='$url2' where rssurl = '$url' ; update rss_item set feedurl='$url2' where feedurl='$url'"
		else
			echo "            not replacing that feed url because feed reply is not recognised as rss content"
		fi
	else
		echo "            not replacing that feed url because feed reply code is not 200"
	fi
	[ -f "$headers" ] && rm "$headers"
	[ -f "$rss" ] && rm "$rss"
}

# replace http with https in feeds urls
feeds=$(grep -cE "^http:" "$u")
i=0
for url in $(shuf "$u" | grep -E "^http:" | awk '{print $1}')
do
	i=$((i+1))
	url2=$(echo "$url" | sed 's/http:/https:/')
	printf "\r\e[K%s/%s %s\n" "$i" "$feeds" "$url"
	_replace
done

# check all feeds return valid http codes
feeds=$(grep -cE "^http" "$u")
i=0
for url in $(shuf "$u" | grep -E "^http" | awk '{print $1}')
do
	i=$((i+1))
	#clear the line before echoing over it
	printf "\r\e[K%s/%s %s\r" "$i" "$feeds" "$url"
	#echo -ne "\r\e[K$i/$feeds $url\r"
	response=$(curl -A "$useragent" --connect-timeout "$timeout" --max-time "$timeout" --write-out "%{http_code}" --silent -D "$headers" --output "$rss" "$url")
	echo curl -A "$useragent" --connect-timeout "$timeout" --max-time "$timeout" --write-out "%{http_code}" --silent -D "$headers" --output "$rss" "$url"
	case "$response" in
		3*)
			#url2=$(curl -A "$useragent" -IL --silent "$url" | awk '/^[lL]ocation: /{ print $2 }' | head -1 | sed 's/\r//g')
			echo "$response [ https://httpstatuses.com/$response ] $url"
			url2=$(awk '/^[lL]ocation: /{ print $2 }' "$headers" | head -1 | sed 's/\r//g')
			case "$url2" in
				http*)
					echo "                            moved to $url2"
					_replace
					;;
				/*)
					domain=$(echo "$url" | awk -F/ '{printf("%s//%s",$1,$3)}')
					url2="$domain$url2"
					echo "                            moved to $url2"
					_replace
					;;
				*)
					printf "\n"
					echo "not replacing that feed url because new feed URL is invalid or incomplete"
					;;
			esac
			;;
		429)
			#oops hammering too many requests
			#uncomment and adjust the sleep timer below if randomising the feeds sequence was not enough to avoid "429" codes replies
			#sleep 60
			;;
		200)
			# feed OK nothing to do
			;;
		*)
			#everything else i.e. 000, 4xx and 5xx could be tagged _fail
			#some 2xx http codes might return valid rss feeds?
			printf "\n"
			echo "$response [ https://httpstatuses.com/$response ] $url may have problems"
			#echo "$response [ https://httpstatuses.com/$response ] $url adding tag: $tagfail"
			#if [ ! "$(grep -cE "^$url $tagfail" "$u")" = 1 ]; then
				#fail tagging disabled for now
				#sed -i "s,$url,$url $tagfail," "$u"
			#fi
			;;
	esac
done

