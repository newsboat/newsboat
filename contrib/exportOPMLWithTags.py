#!/usr/bin/env python3

# this script exports the urls file to OPML, including tags. for that, all feeds must have only one tag

#usage: ./exportOPMLWithTags.py urls > urls.opml

#requeriments (just to get the title from a rss feed if it isn't cached in Newsboat):
# pip install feedparser

#input-output example:
#
# $ cat urls
# https://xkcd.com/rss.xml "must"
# http://www.commitstrip.com/en/feed/? "must"
# https://jartigag.xyz/feed.xml "personal"
#
# $ cat urls.opml
# <?xml version="1.0" ?>
# <opml version="2.0">
#   <head/>
#   <body>
#     <outline title="must" type="rss">
#       <outline title="xkcd.com" type="rss" xmlUrl="https://xkcd.com/rss.xml"/>
#       <outline title="CommitStrip" type="rss" xmlUrl="http://www.commitstrip.com/en/feed/?"/>
#     </outline>
#     <outline title="personal" type="rss">
#       <outline title="jartigag" type="rss" xmlUrl="https://jartigag.xyz/feed.xml"/>
#     </outline>
#   </body>
# </opml>

from xml.etree import ElementTree as ET
from xml.dom import minidom
import sys, os
import csv
import sqlite3

try:

    if len(sys.argv)<2: raise Exception("input file needed")

    inputfilename = sys.argv[1]

    if not os.path.isfile(inputfilename): raise Exception(f"{inputfilename} not found")

    with open(inputfilename) as f:
        reader = csv.reader(f,delimiter=" ")
        lines = list(reader)

    root = ET.Element('opml', version='2.0')
    head = ET.SubElement(root, 'head')
    body = ET.SubElement(root, 'body')

    try:
        # get titles from cache.db
        cache_path = f"{os.environ['HOME']}/.local/share/Newsboat/cache.db"
        if not os.path.exists(cache_path):
            cache_path = f"{os.environ['HOME']}/.Newsboat/cache.db"

        with sqlite3.connect(cache_path) as conn:
            conn.row_factory = sqlite3.Row
            c = conn.cursor()
            c.execute("select rssurl,title from rss_feed")
            db_rows = c.fetchall()
    except sqlite3.OperationalError:
        pass


    for line in lines:

        if len(line)<2 or line[0].startswith("#"):
            # Ignore lines that are not `url "tag"` (at least 2 parts)
            # and ignore all comments in the file
            print(f"ignoring this line:\n{' '.join(line)}", file=sys.stderr)
            continue

        if not body:
        # that is, body has no children
            tag = ET.SubElement(body, 'outline', type='rss', title=line[1])
        elif line[1] not in [ o.attrib['title'] for o in body.findall('outline') ]:
        # that is, this tag doesn't exist yet
            tag = ET.SubElement(body, 'outline', type='rss', title=line[1])

        for tag in body.findall('outline'):
            if tag.attrib['title']==line[1]:
            # that is, this is the tag we are looking for
                feed = ET.SubElement(tag, 'outline', type='rss', xmlUrl=line[0])
                for row in db_rows:
                # look for the title among the cached ones
                    if row['rssurl']==line[0]:
                        feed.set('title', row['title'])
                if 'title' not in feed.attrib:
                # that is, this feed's title isn't in ~/.Newsboat/cache.db
                    try:
                        import feedparser
                        print(f"getting title from {line[0]}", file=sys.stderr)
                        feed.set('title', feedparser.parse(line[0])['feed']['title'])
                    except (ModuleNotFoundError, KeyError):
                        # can't get title neither from cache.db nor the xml of the feed,
                        # so left title blank
                        feed.set('title', "")

    print(
        minidom.parseString( ET.tostring(root) )
        .toprettyxml(indent="  ")
    )

except Exception as e:
    print(e)
