#!/bin/sh

subscription_capture_1='s|<div class="subscription-widget-wrap" .*<div class="fake-button"></div></div></form></div></div>||g'
subscription_capture_2='s|<p class="button-wrapper" [^>]*><a class="button primary" href="https://[a-z0-9]*.substack.com/subscribe?"><span>Subscribe now</span></a></p>||g'
share_capture_1='s|<a class="button primary" href="https://[^>]*><span>Share</span></a>||g'
ps_capture='s|<p></p>||g'

cat |  xmllint --format - | sed "$subscription_capture_1" | sed "$subscription_capture_2" | sed "$share_capture_1" | sed "$ps_capture"
## see <https://stackoverflow.com/questions/19408649/pipe-input-into-a-script>
## https://stackoverflow.com/questions/16090869/how-to-pretty-print-xml-from-the-command-line
