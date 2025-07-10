#!/usr/bin/env python3

"""
Script for moving feed URLs in Newsboat's database/cache file while
preserving articles. The script does not modify the urls file nor does
it modify any other field (links in articles and feed titles are unmodified)

2023 blankie <blankie@nixnetmail.com>
"""

import os
import sys
import typing
import sqlite3
import argparse

def move_url(cursor: sqlite3.Cursor, oldurl: str, newurl: str):
    cursor.execute("UPDATE rss_item SET feedurl = ? WHERE feedurl = ?",
        (newurl, oldurl))
    cursor.execute("UPDATE rss_feed SET rssurl = ? WHERE rssurl = ?",
        (newurl, oldurl))

def _get_cache_path() -> typing.Optional[str]:
    if "XDG_DATA_HOME" in os.environ:
        return os.path.join(os.environ["XDG_DATA_HOME"],
            "Newsboat", "cache.db")

    unexpanded_path = os.path.join("~", ".Newsboat", "cache.db")
    expanded_path = os.path.expanduser(unexpanded_path)
    if expanded_path != unexpanded_path:
        return expanded_path

    return None

def _try_lock(lock_file) -> bool:
    try:
        os.lockf(lock_file.fileno(), os.F_TLOCK, 0)
    except (BlockingIOError, PermissionError):
        return False

    lock_file.truncate()
    lock_file.write(str(os.getpid()))
    lock_file.flush()
    return True

def _main() -> int:
    default_cache_path: typing.Optional[str] = _get_cache_path()

    parser: argparse.ArgumentParser = argparse.ArgumentParser()
    parser.add_argument("-c", "--cache-file", help="path to database file",
        default=default_cache_path, required=default_cache_path is None)
    parser.add_argument("oldurl", help="url to move from")
    parser.add_argument("newurl", help="url to move to")
    args: argparse.Namespace = parser.parse_args()

    lock_path: str = args.cache_file + ".lock"
    lock_file: typing.TextIO = open(lock_path, "w+")
    try:
        if not _try_lock(lock_file):
            locker_pid = lock_file.read(1024)
            print("Error: the database is opened by another process "
                f"(PID: {locker_pid})", file=sys.stderr)
            return 1

        connection: sqlite3.Connection = sqlite3.connect(args.cache_file)
        move_url(connection.cursor(), args.oldurl, args.newurl)
        connection.commit()
        print("URL moved successfully. Remember to replace the old url in "
            "your URLs file to the new one.")
    finally:
        try:
            os.remove(lock_path)
        except OSError:
            pass

    return 0

if __name__ == "__main__":
    sys.exit(_main())
