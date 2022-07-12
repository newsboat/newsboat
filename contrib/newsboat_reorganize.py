#!/usr/bin/env python

"""
Script for organizing newsboats `urls` file. Opens up the file in a text
editor with the feeds titles added for easy organization. (Be sure to leave in
the added markers)

2022 Mustafa Ibrahim <h9s@tutanota.com>
"""

import sys
import os
import subprocess
import tempfile
import re
import xml.etree.ElementTree as ET
from urllib.request import urlopen

def get_newsboat_urls_path():
    try:
        newsboat_urls_path = os.environ['XDG_CONFIG_HOME'] + '/newsboat/urls'
    except KeyError:
        newsboat_urls_path = os.environ['HOME'] + '/.config/newsboat/urls'
    return newsboat_urls_path

def get_xml_title(url):
    title = None
    try:
        root = ET.parse(urlopen(url))
        title = (root.find('.//{*}title').text)
    except:
        pass
    return title

def add_titles(urls_list):
    for i, line in enumerate(urls_list):
        url = re.search(r'^https?:\S+', line)
        if url is not None:
            title = get_xml_title(url.group(0))
            if title is not None:
                urls_list[i] = title + ' <++> ' + line
    return urls_list

def open_in_editor(contents):
    with tempfile.NamedTemporaryFile(mode='r+') as tmp:
        tmp.writelines(contents)
        tmp.flush()
        try:
            editor = os.environ['EDITOR']
        except KeyError:
            editor = 'vi'
        subprocess.call([editor, tmp.name])
        tmp.seek(0)
        contents = tmp.readlines()
        return contents

def remove_titles(titled_urls):
    for i, line in enumerate(titled_urls):
        titled_urls[i] = re.split(r'^[^#].+?<\+\+>\s?', line)[-1]
    return titled_urls

def main():
    newsboat_urls_path = get_newsboat_urls_path()
    if not os.path.exists(newsboat_urls_path):
        print('Could not find newsboat urls file')
        sys.exit(1)

    with open(newsboat_urls_path, encoding='utf-8') as newsboat_urls:
        newsboat_urls_data = newsboat_urls.readlines()

    print('Please wait...')
    titled_urls = add_titles(newsboat_urls_data)
    edited_urls = open_in_editor(titled_urls)

    proceed = input(f'Do you want to write the changes to \'{newsboat_urls_path}\'? [y/N] ')
    if proceed.lower() in ['y', 'yes']:
        with open(newsboat_urls_path, 'w', encoding='utf-8') as newsboat_urls:
            new_newsboat_urls = remove_titles(edited_urls)
            newsboat_urls.writelines(new_newsboat_urls)

if __name__ == '__main__':
    main()
