How to cut a Newsboat release
-----------------------------

This document describes all the steps one should go through in order to make
a new release. The person doing the release should have push access to the
master repository, shell access to newsboat.org, and access to the Snap
dashboard.


## If you're making a minor release (x.Y.z)

0. Pull all the latest changes from the main repo.
1. Run `git log --reverse PREVIOUS_VERISION..`, search for "^Merge".
2. Proceed to the "Common steps" section.


## If you're making a patch release (x.y.Z)

Since the 2.19 release we have a schedule for bumping the minimum supported Rust
version (MSRV). However, we also promise that patch releases will have the same
MSRV as the initial release, i.e. for any Z in X.Y.Z the MSRV is the same. This
makes the process for a patch release a bit different from the ordinary release;
instead of prepping and pushing the contents of the `master` branch, we have to
branch off the latest release and backport the bugfixes onto it.


0. Open this document in Vim, add an empty line at the end, but **do not save
   it**.

   The point is to have an up-to-date version open. Modified buffer won't be
   automatically reloaded when you switch branches later on, i.e. Vim won't pick
   up the document version from an earlier commit. And since it's open in an
   editor, you can polish the document as you perform the steps.

1. Open the list of PRs that you earmarked for backporting.

2. Branch off the latest release:

        $ git checkout -b feature/2.22.1 r2.22

3. Backport the changes.
    * Use `git cherry-pick -x COMMIT` -- this appends "(cherry picked from
        commit COMMIT)" to the commit message.

4. Proceed to the "Common steps" section.


## Common steps

0. Update copyright years:
    * `git grep -- -2023` to find potentially outdated copyrights
    * fix them
    * `git commit -am'Bump copyright notices'`
0. Update CHANGELOG:
    * Go through the list you already opened.
    * Mention issue number ("#X" for Newsboat issues, full link to issue tracker
        for Newsbeuter issues).
    * Mention the name of the contributor.
    * Acknowledge contributions from people whose changes didn't make it into
        the lists. The full list of contributors can be obtained with:
        ```
        $ git shortlog -s PREVIOUS_VERSION.. | cut -f2
        ```
1. Update version:
    * _rust/libnewsboat/Cargo.toml_
    * _rust/libnewsboat-ffi/Cargo.toml_
    * `cargo update --package libnewsboat --package libnewsboat-ffi`.
2. Update links to docs and FAQ in README (do it with a regex, there's lots of
   them).
3. Commit the changes with message "Release VERSION".
4. *If you're making a patch release*, push the branch and wait for CI to succeed
   before proceeding.
4. Create a new tag:
    * `git tag --sign -u 'newsboat@googlegroups.com' rVERSION`.
    * First line: "Release Newsboat VERSION".
    * Description: copy of the changelog entry.
        * Don't use "###" style for headers because they'll be stripped ("#" is
            a shell comment). Use "===" style instead.
5. Prepare the tarball:
    * `git archive --format=tar --prefix="newsboat-VERSION/" rVERSION | pixz > newsboat-VERSION.tar.xz`.
    * Sign the tarball:
        `gpg2 --sign-with 'newsboat@googlegroups.com' --detach-sign --armour newsboat-VERSION.tar.xz`.
6. Prepare the docs:
    * In your local clone: `make -j5 doc`.
7. *If you're making a patch release*, merge the tag into the master branch:

        $ git checkout master
        $ git merge r2.22.1

    This merges our changes to CHANGELOG.

8. Publish the release:
    * Push the code: `git push && git push --tags`
    * Update the site:
        * Navigate to your local clone of https://github.com/newsboat/newsboat.org
        * Prepare directories: `mkdir -p www/releases/VERSION/docs`.
        * Move tarball and its signature:
            `mv newsboat-VERSION* www/releases/VERSION/`.
        * Move docs:
            `mv NEWSBOAT/doc/xhtml/faq.html NEWSBOAT/doc/xhtml/newsboat.html www/releases/VERSION/docs/`.
        * Compress docs:
            `gzip --keep --best www/releases/VERSION/docs/*`.
        * Edit `www/index.html`:
            * Move current release to the list of previous releases.
            * Update current release version.
            * Update current release date.
            * Update current release links.
            * Update the year in the page copyright if necessary.
            * Gzip the result: `gzip --best --keep --force www/index.html`.
        * Edit `www/news.atom`:
            * Update `<updated>` field of the channel.
            * Use the same date-time for `<published>` and `<updated>` in new
                `<entry>`.
            * Update entry's `<link>` and `<id>` to point to new docs'
                `newsboat.html`.
            * `<title>`: "Newsboat VERSION is out".
            * Gzip the result: `gzip --best --keep --force www/news.atom`.
            * Commit the result: `git commit -m'Release VERISON'`
            * Publish it: `git push`
8. Save the website to the Wayback machine:
    1. go to https://web.archive.org/save
    2. type in "newsboat.org"
    3. enable "Save outlinks" so the Web Archive downloads the tarballs and
       whatnot
    4. click "Save page" and wait for it to finish, making sure there were no
       errors
8. Tell the world about it:
    * Send an email to the mailing list
        * newsboat@googlegroups.com
        * Same topic and contents as in `news.atom` entry.
        * Clear-sign instead of detach-sign: select the body and run it through
            `gpg2 --clearsign`.
    * Change the topic on #newsboat at irc.libera.chat
9. Release the snap:
    * For minor releases:
        * Go to https://snapcraft.io/newsboat/releases and drag the line with
            the latest release onto "stable", "candidate", and "beta" lines;
            then click "Save"
    * For patch releases:
        * Follow _howto-update-snap.markdown_ to build and release a new version.
        * Run `git fetch origin --tags` to fetch the tag you just pushed.
10. Manage milestones https://github.com/newsboat/newsboat/milestones?with_issues=no :
    * Add all unassigned issues and pull requests to the current one:
        * Search for "no:milestone closed:>=2020-03-20 is:pr state:merged", set
            milestone for all.
        * Search for "no:milestone closed:>=2020-03-20 is:pr state:closed",
            check if any of them are actually merged manually.
        * Search for "no:milestone closed:>=2020-03-20 is:issue", set milestone
            on those that are actually fixed or at least affected by the release.
    * Close the current milestone.
    * Create a new milestone with a date set to 20-ish of
        March/June/September/December.


## If you're making a minor release (x.Y.z)

1. Add a reminder to myself to update the POT file and ask for
   updated translations two weeks before the next release
2. Prepare the repo for the next release:
    * Add "Unreleased" section to CHANGELOG.
    * Update all the transitive dependencies: `cargo update`.
    * Set minimum supported Rust version (MSRV) to current_stable-2:
        * `git grep <previous version>`;
        * includes CI configs, README and doc/newsboat.asciidoc;
        * mention this in "Changed" section of the changelog.
    * Commit and submit a PR:

        git switch -c feature/prepare-next-release
        git commit -am'Prepare for next release'
        git push origin -u feature/prepare-next-release
3. If it's September, bump the expiry date on the OpenPGP key:

    * edit the key, upload it to keyservers and export it to a file:

        $ gpg --edit-key 4ED6CD61932B9EBE
        gpg> key 0
        gpg> expire
        Key is valid for? (0) 54w

        gpg> key 1
        gpg> expire
        Key is valid for? (0) 54w

        gpg> save
    * upload it to the keyserver:
        `gpg --keyserver keys.openpgp.org --send-keys 4ED6CD61932B9EBE`
    * export it to a file:
        `gpg --armour --export 4ED6CD61932B9EBE > newsboat.pgp`
    * upload the file to newsboat.org staging area
    * on newsboat.org, put the key into the www directory

        sudo chown www-data:www-data newsboat.pgp
        sudo chmod u=rw,go=r newsboat.pgp
        sudo mv newsboat.pgp /var/www/newsboat.org/www/newsboat.pgp


## If you're making a patch release (x.y.Z)

1. Remove the branch you made to backport stuff:

        $ git branch --delete --force feature/2.22.1
        $ git push origin --delete feature/2.22.1
