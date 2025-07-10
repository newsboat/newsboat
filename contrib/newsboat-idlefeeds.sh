#!/bin/sh


USAGE="
Displays feeds that havent been updated for a certain ammount of time
    $ Newsboat-idlefeeds.sh # default 6 months
    $ Newsboat-idlefeeds.sh -c ./cache.db # specify cache file
    $ Newsboat-idlefeeds.sh -t '1 year' # specify other duration
"

cache="${XDG_DATA_HOME:-$HOME/.local/share}/Newsboat/cache.db"
time="6 month"

while getopts hc:t: flag; do
    case $flag in
	c) cache=$OPTARG ;;
	t) time=$OPTARG ;;
	h|*) echo "$USAGE"; exit 1 ;;
    esac
done
shift "$((OPTIND - 1))"


sqlite3 -header -column "$cache" "
SELECT
    datetime(MAX(rss_item.pubDate), 'unixepoch') AS last_pubDate,
    rss_feed.title,
    rss_item.feedurl
FROM
    rss_item
JOIN
    rss_feed
ON
    rss_item.feedurl = rss_feed.rssurl
GROUP BY
    rss_item.feedurl
HAVING
    datetime(MAX(rss_item.pubDate), 'unixepoch') < DATE('now', '-$time')
ORDER BY
    MAX(rss_item.pubDate) ASC;
" | less
