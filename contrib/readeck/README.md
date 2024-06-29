# Intention
These scripts will allow you to create bookmarks for the [Readeck](https://readeck.org/en/) bookmark manager.

# Requirements
- [Babashka](https://github.com/babashka/babashka#installation)
  These are clojure programs and using babashka to run them instead of JVM Clojure means that you get much faster startup times.

# Usage
Edit the script that you want with your installation to fill in:

1) Your readeck address: If you're hosting it on your local machine it might be "http://localhost:8000"
   Alternatively, if you're using a custom domain, it would be that (with no port number).

2) Your API token: You can create an API token at <your-readeck-address>/profile/tokens
   Whatever token you generate will need to have the **Bookmarks: Write Only** permission

3) Default-tags: Both scripts give you the option of adding default tags.
   In the case of the standard-readeck.clj the default tags will **always** be added
   In the case of the interactive-labels-readeck.clj they will only be added if no alternative tags are supplied when prompted.

To accept the default title and url suggestions when creating a bookmark, you can add
`bookmark-autopilot yes` to your config file.


## Standard Script
If you'd like to save bookmarks without adding labels (ie tags) to them, then
you would set
`bookmark-cmd` `standard-readeck.clj` in your newsdeck config file


## Interactive Script (with labels)
If you'd like to add labels when you add bookmarks, then you would set
`bookmark-cmd` to be `interactive-labels-readeck.clj` in your newsdeck config file.

Now, calling the bookmark command will allow you to add a comma separated list of labels
that are submitted with the <Return> key.
