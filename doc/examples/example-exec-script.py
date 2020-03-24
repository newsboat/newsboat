#!/usr/bin/env python3

# This script can be used by adding a line like the following to the Newsboat urls file:
# "exec:~/example-exec-script.py \"first argument\" \"<argument> containing \\\"literal\\\" quote\"" "this-is-a-tag" "generated-with-python"
# Update `~/example-exec-script.py` to the actual location where you placed this script.

import sys
import xml.etree.ElementTree as ET

feed = {
    "title": "Python RSS generator test feed",
    "link": "https://example.com/whatever",
    "description": "Describe your feed",
    # When adding feed properties, update `create_channel_information`
}

items = [
    {
        "title": "This is an article title",
        "link": "https://example.com/articles/1",
        "guid": {
            "value": "http://example.com/articles/1",
        },
        "description": "Lorem ipsum dolar sit amit.<br>You can add <a href=\"https://example.com/test>links</a> like usual in HTML."
        # When adding item properties, update `create_item`
    },
    {
        "title": "A minimalistic second item",
        "guid": {
            "value": "should be unique",
            "isPermaLink": False, # See https://validator.w3.org/feed/docs/rss2.html#ltguidgtSubelementOfLtitemgt
        },
    },
]

def escape_for_html(text):
    return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

# Add a dynamic item which shows the arguments passed to this script (might be useful to debug quoting in the "exec:…" line)
if len(sys.argv) > 0:
    item_text = "This script was called with the following arguments:"

    for i in range(len(sys.argv)):
        item_text += "<br>"
        item_text += "- sys.argv[{}]: {}".format(i, escape_for_html(sys.argv[i]))

    items.append({
        "title": "Arguments to the script (Python sys.argv)",
        "description": item_text,
        "link": "file://" + sys.argv[0],
        "guid": {
            "value": "sys.argv",
            "isPermaLink": False,
        },
    })

def create_channel_information(channel_info):
    channel = ET.Element("channel")
    # required elements:
    # - title: The name of the channel.
    # - link: The URL to the HTML website corresponding to the channel.
    # - description: Phrase or sentence describing the channel.
    for name in ["title", "link", "description"]:
        ET.SubElement(channel, name).text = channel_info[name]

    # Optional elements: https://validator.w3.org/feed/docs/rss2.html#optionalChannelElements
    return channel

def create_item(item_info):
    item = ET.Element("item")
    # The rss specification requires that at least one of "title" or "description" is present
    # Newsboat by default shows the title in the article list so it is a good idea to always include a title
    ET.SubElement(item, "title").text = item_info["title"]
    # All other elements of the item are optional: https://validator.w3.org/feed/docs/rss2.html#hrelementsOfLtitemgt
    if "link" in item_info:
        ET.SubElement(item, "link").text = item_info["link"]
    if "description" in item_info:
        ET.SubElement(item, "description").text = item_info["description"]
    if "guid" in item_info and "value" in item_info["guid"]:
        guid = item_info["guid"]
        attrs = {}
        if "isPermaLink" in guid and not guid["isPermaLink"]:
            attrs["isPermaLink"] = "false"
        ET.SubElement(item, "guid", attrs).text = item_info["guid"]["value"]

    return item

rss_root = ET.Element("rss", attrib={"version": "2.0"})
channel = create_channel_information(feed)
for item in items:
    rss_item = create_item(item)
    channel.append(rss_item)
rss_root.append(channel)

rss_xml = ET.tostring(rss_root, encoding='unicode', method='xml')
print(rss_xml)
