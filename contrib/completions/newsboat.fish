complete -c newsboat -s e -l export-to-opml -d "export OPML feed to stdout"
complete -c newsboat -l export-to-opml2 -d "export OPML 2.0 feed including tags to stdout"
complete -c newsboat -s r -l refresh-on-start -d "refresh feeds on start"
complete -c newsboat -s i -l import-from-opml -d "import OPML file" -r
complete -c newsboat -s u -l url-file -d "read RSS feed URLs from file" -r
complete -c newsboat -s c -l cache-file -d "use specified file as cache file" -r
complete -c newsboat -s C -l config-file -d "use specified file as config file" -r
complete -c newsboat -l queue-file -d "use specified file as podcast queue file" -r
complete -c newsboat -l search-history-file -d "save the input history of the search to specified file" -r
complete -c newsboat -l cmdline-history-file -d "save the input history of the command line to specified file" -r
complete -c newsboat -s X -l vacuum -d "compact the cache"
complete -c newsboat -s x -l execute -d "execute list of commands and exit" -xa 'reload\t"reload all feeds" print-unread\t"print amount of unread articles"'
complete -c newsboat -s q -l quiet -d "reduce output at startup"
complete -c newsboat -s v -l version -d "print version information and exit"
complete -c newsboat -s l -l log-level -d "write log file including everything at and above the given level" -xa '1\t"user error" 2\t"critical" 3\t"error" 4\t"warning" 5\t"info" 6\t"debug"'
complete -c newsboat -s d -l log-file -d "use specified file as output log file" -r
complete -c newsboat -s E -l export-to-file -d "export list of read articles to file" -r
complete -c newsboat -s I -l import-from-file -d "import list of read articles from file" -r
complete -c newsboat -s h -l help -d "show help and exit"
complete -c newsboat -l cleanup -d "remove unreferenced items from cache"
