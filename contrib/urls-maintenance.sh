#!/bin/sh
# operates various maintenance tasks on urls and cache.db for newsboat:
# - convert http to https in urls file when https feed is available
# - convert feeds urls to their new forwarded urls for 3xx http codes
# - feeds urls not returning a valid http code 200 are tagged "_dead"
# cache.db is also updated to refer to the new url feeds so all read/unread articles and flags are preserved

# TODO
# implement additional checks for "valid" feeds returning 200 http code: 
# https://www.linuxjournal.com/content/parsing-rss-news-feed-bash-script
#	is it returning valid rss?
#	when was the feed last updated?
#	sort valid feeds by last updated
#	tag feed "abandoned" when feed not updated for more than 1 year


#newsboat urls file and cache locations
u="$HOME/.config/newsboat/urls"
db="$HOME/.config/newsboat/cache.db"
#curl timeout for testing URLs
timeout=10
tagdead="_dead"

# shuf (GNU coreutils) randomises the urls list, this avoids querying the same domains too fast, assuming urls are grouped by domains or alphasorted in the urls file
requirements=( newsboat curl sqlite3 sed grep awk head shuf command )
for app in ${requirements[*]}
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
	echo "newsboat is still running. Close it first then try again"; exit
fi

cp "$db" "$db.bak-$(date +%FT%T)"
cp "$u" "$u.bak-$(date +%FT%T)"

feeds=$(wc -l "$u" | awk '{print $1}')
i=0
# check all feeds return valid http codes
for url in $(shuf "$u" | awk '{print $1}')
do
	i=$((i+1))
	#clear the line before echoing over it
	echo -ne "\r\e[K$i/$feeds $url\r"
	response=$(curl -I --connect-timeout "$timeout" --write-out "%{http_code}" --silent --output /dev/null "$url")
	case "$response" in
		3*)
			url2=$(curl -IL --silent "$url" | awk '/^[lL]ocation: /{ print $2 }' | head -1 | sed 's/\r//g')
			echo -ne "\n"
			echo "$response [ https://httpstatuses.com/$response ] $url"
			echo "                            moved to $url2"
			case "$url2" in
				http*)
					sed -i "s,^$url,$url2," "$u"
					sqlite3 "$db" "update rss_feed set rssurl='$url2' where rssurl = '$url' ; update rss_item set feedurl='$url2' where feedurl='$url'"
					;;
				/*)
					domain=$(echo "$url" | awk -F/ '{printf("%s//%s",$1,$3)}')
					url2="$domain$url2"
					sed -i "s,^$url,$url2," "$u"
					sqlite3 "$db" "update rss_feed set rssurl='$url2' where rssurl = '$url' ; update rss_item set feedurl='$url2' where feedurl='$url'"
					;;
				*)
					echo -ne "\n"
					echo "not replacing that feed url because new feed URL invalid or incomplete"
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
			#everything else i.e. 000, 4xx and 5xx could be tagged _dead
			#some 2xx http codes might return valid rss feeds?
			echo -ne "\n"
			echo "$response [ https://httpstatuses.com/$response ] $url adding tag: $tagdead"
			if [[ ! $(grep "$url" "$u") == *" $tagdead"* ]]; then
				sed -i "s,$url ,$url $tagdead ," "$u"
			fi
			;;
	esac
done

# replace http with https in feeds urls
feeds=$(grep -c "http:" "$u")
i=0
for url in $(shuf "$u" | grep "http:" | awk '{print $1}')
do
	i=$((i+1))
	#url2=$(echo "$url" | sed 's/http:/https:/')
	url2=${url/http:/https:}
	response=$(curl -IL --connect-timeout "$timeout" --write-out "%{http_code}" --silent --output /dev/null "$url2")
	echo "$i/$feeds $response https reply on $url"
	if [ "$response" == "200" ]; then
		sed -i "s,^$url,$url2," "$u"
		sqlite3 "$db" "update rss_feed set rssurl='$url2' where rssurl = '$url' ; update rss_item set feedurl='$url2' where feedurl='$url'"
	else
		echo "            not replacing that feed url because feed reply code is not 200"
	fi
done

