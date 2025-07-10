complete -c Newsboat -s e -l export-to-opml -d "export OPML feed to stdout"
complete -c Newsboat -l export-to-opml2 -d "export OPML 2.0 feed including tags to stdout"
complete -c Newsboat -s r -l refresh-on-start -d "refresh feeds on start"
complete -c Newsboat -s i -l import-from-opml -d "import OPML file" -r
complete -c Newsboat -s u -l url-file -d "read RSS feed URLs from file" -r
complete -c Newsboat -s c -l cache-file -d "use specified file as cache file" -r
complete -c Newsboat -s C -l config-file -d "use specified file as config file" -r
complete -c Newsboat -l queue-file -d "use specified file as podcast queue file" -r
complete -c Newsboat -l search-history-file -d "save the input history of the search to specified file" -r
complete -c Newsboat -l cmdline-history-file -d "save the input history of the command line to specified file" -r
complete -c Newsboat -s X -l vacuum -d "compact the cache"
complete -c Newsboat -s x -l execute -d "execute list of commands and exit" -xa 'reload\t"reload all feeds" print-unread\t"print amount of unread articles"'
complete -c Newsboat -s q -l quiet -d "reduce output at startup"
complete -c Newsboat -s v -l version -d "print version information and exit"
complete -c Newsboat -s l -l log-level -d "write log file including everything at and above the given level" -xa '1\t"user error" 2\t"critical" 3\t"error" 4\t"warning" 5\t"info" 6\t"debug"'
complete -c Newsboat -s d -l log-file -d "use specified file as output log file" -r
complete -c Newsboat -s E -l export-to-file -d "export list of read articles to file" -r
complete -c Newsboat -s I -l import-from-file -d "import list of read articles from file" -r
complete -c Newsboat -s h -l help -d "show help and exit"
complete -c Newsboat -l cleanup -d "remove unreferenced items from cache"
