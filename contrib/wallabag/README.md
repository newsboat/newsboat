# README

A simple Wallabag plugin.

1. `gem install httparty`
2. Edit to reflect your Wallabag instance's base URL. (e.g. `https://wallabag.example.com/`)
3. Get your Wallabag API tokens from `Settings > API clients management` in your Wallabag instance
4. Either set the following environment variables with your tokens from step 3, or hardcode them in this script:
   - `WALLABAG_CLIENT`
   - `WALLABAG_SECRET`
5. Set `WALLABAG_USER` and `WALLABAG_PASSWORD`, either as environment variables or hardcode them.
5. Copy somewhere you can find it. (`~/bin`, `~/usr/local/bin`, etc.)
6. Set `bookmark-cmd` to `~/bin/wallabag.rb` or wherever you put this.
