#!/bin/bash

sed_capture='s|<div class="subscription-widget-wrap" .*<div class="fake-button"></div></div></form></div></div>||g'
sed_capture_2='s|<a class="button primary" href="https://[a-z0-9]*.substack.com/subscribe?"><span>Subscribe now</span></a></p>||g'
cat |  xmllint --format - | sed "$sed_capture" | sed "$sed_capture_2"

## see <https://stackoverflow.com/questions/19408649/pipe-input-into-a-script> 
## https://stackoverflow.com/questions/16090869/how-to-pretty-print-xml-from-the-command-line
