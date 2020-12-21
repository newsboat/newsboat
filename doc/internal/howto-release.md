How to cut a Newsboat release
-----------------------------

This document describes all the steps one should go through in order to make
a new release. The person doing the release should have push access to the
master repository and shell access to newsboat.org.

0. Pull all the latest changes from the main repo.
1. Update CHANGELOG
    * Consult `git log --reverse PREVIOUS_VERISION..`, search for " Merge "
    * Mention issue number ("#X" for Newsboat issues, full link to issue tracker
        for Newsbeuter issues)
    * Mention the name of contributor
    * Acknowledge contributions from people whose changes didn't make it into
        the lists. The full list of contributors can be got with:
        ```
        $ git shortlog PREVIOUS_VERSION.. | egrep -v '^(\s|$) | sed -r 's# \([[:digit:]]+\):$##''
        ```
2. Update version:
    * rust/libnewsboat/Cargo.toml
    * rust/libnewsboat-ffi/Cargo.toml
    * `cargo update --package libnewsboat --package libnewsboat-ffi`
3. Update links to docs and FAQ in README
4. Commit the changes with message "Release VERSION"
5. Create new tag:
    * `git tag --sign -u 'newsboat@googlegroups.com' rVERSION`
    * First line: "Release Newsboat VERSION"
    * Description: copy of changelog entry
        * Don't use "###" style for headers because they'll be stripped ("#" is
            a shell comment). Use "===" style instead
6. Prepare the tarball
    * `git archive --format=tar --prefix="newsboat-VERSION/" rVERSION | pixz
        > newsboat-VERSION.tar.xz`
    * Sign the tarball:
        `gpg2 --sign-with 'newsboat@googlegroups.com' --detach-sign --armour newsboat-VERSION.tar.xz`
    * Upload both files to newsboat.org staging area
7. Prepare the docs
    * In your local clone: `make -j5 doc`
    * Upload contents of `doc/xhtml/` to newsboat.org staging area
8. Publish the release
    * Prepare the directory on the server
        * `cp -rfv /usr/local/www/newsboat.org/www/ newsboat`
        * Prepare directories: `mkdir -p newsboat/releases/VERSION/docs`
        * Move tarball and its signature:
            `mv newsboat-VERSION* newsboat/releases/VERSION/`
        * Prepare docs:
            `gzip --keep --best faq.html newsboat.html`
        * Move docs:
            `mv faq.html* newsboat.html* newsboat/releases/VERSION/docs/`
        * Edit `newsboat/index.html`
            * Move current release to the list of previous releases
            * Update current release version
            * Update current release date
            * Update current release links
            * Update the year in the page copyright if necessary
            * Gzip the result: `gzip --best --keep --force newsboat/index.html`
        * Edit `newsboat/news.atom`
            * Update `<updated>` field of the channel
            * Use the same date-time for `<published>` and `<updated>` in new
                `<entry>`
            * Update entry's `<link>` and `<id>` to point to new docs'
                `newsboat.html`
            * `<title>`: "Newsboat VERSION is out"
            * Gzip the result: `gzip --best --keep --force newsboat/news.atom`
    * Prepare an email to the mailing list
        * Same topic and contents as in `news.atom` entry
        * Clear-sign instead of detach-sign: select the body and run it through
            `gpg2 --clearsign`
    * Deploy the directory on the server:
        `sudo cp -rv newsboat/ /usr/local/www/newsboat.org/www/ && sudo chmod -R a+r /usr/local/www/newsboat.org/www/`
    * Push the code: `git push && git push --tags`
9. Tell the world about it
    * Send an email to the mailing list
    * Change the topic on #newsboat at Freenode
    * Post something about it on personal Mastodon
10. Release the snap
    * Go to https://dashboard.snapcraft.io/ and log in
    * Go to https://dashboard.snapcraft.io/snaps/newsboat/revisions/ and for
        each revision that corresponds to the release:
        * Click on the revision number
        * In "Channels" row, click "Release"
        * Tick all checkboxes ("stable", "candidate", "beta", and "edge")
        * Click "Release"
11. Manage milestones https://github.com/newsboat/newsboat/milestones?with_issues=no
    * Add all unassigned issues and pull requests to the current one:
        * Search for "no:milestone closed:>=2020-03-20 is:pr state:merged", set
            milestone for all
        * Search for "no:milestone closed:>=2020-03-20 is:pr state:closed",
            check if any of them are actually merged manually
        * Search for "no:milestone closed:>=2020-03-20 is:issue", set milestone
            on those that are actually fixed or at least affected by the release
    * Close the current milestone
    * Create a new milestone with a date set to 20-ish of
        March/June/September/December
12. Add a TaskWarrior reminder to myself to update POT file and ask for updated
    translations two weeks before the next release
13. Prepare the repo for the next release
    * Add "Unreleased" section to CHANGELOG and commit
    * Update all the transitive dependencies: `cargo update` and commit
    * Set minimum supported Rust version (MSRV) to current_stable-2
        * `git grep <previous version>`
        * includes CI configs, README and doc/newsboat.asciidoc
        * commit
    * Push it: `git push`
