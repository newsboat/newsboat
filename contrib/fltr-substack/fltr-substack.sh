#!/bin/bash

sed_capture='s|<div class="subscription-widget-wrap" .*<div class="fake-button"></div></div></form></div></div>||g'
cat |  xmllint --format - | sed "$sed_capture"
## see <https://stackoverflow.com/questions/19408649/pipe-input-into-a-script> 
## https://stackoverflow.com/questions/16090869/how-to-pretty-print-xml-from-the-command-line
