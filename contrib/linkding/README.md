# README

A simple Linkding plugin. 

1. `gem install httparty`
2. Edit to reflect your Linkding instance's API endpoint. (e.g. `http://example.com/api/bookmarks`)
3. Get your Linkding API key from `Settings > Integrations` in your Linkding instance 
4. Either set the environment variable `LINKDING_TOKEN` with your API key from step 3, or hardcode your API key in this script by setting the `token` variable on line 10.
5. Copy somewhere you can find it. (`~/bin`, `~/usr/local/bin`, etc.)
5. Set `bookmark-cmd` to `~/bin/linkding.rb` or wherever you put this.  
