#!/usr/bin/env perl

use strict;
use LWP::UserAgent;
use URI::Escape;


######## User settings #############
# Daemonise the bookmarking process
# Use this only if you are sure things
# are working as expected. Needs the
# Proc::Daemon Perl module
# set to '0' if not needed
my $daemon = 1;


# Change the tag value to what you
# want the bookmarks to be tagged as
# at Pinboard
my $tag='newsbeuter';

# Get yours at https://pinboard.in/settings/password
# Of the form 'username:alphanumeric'
my $API_token='***REPLACE***';
######## No user settings below this line #############


# $ARGV[0] is the URL of the article
# $ARGV[1] is the title of the article
# $ARGV[2] is the description of the article
# $ARGV[3] is the feed title (not used by Pinboard)

if ($daemon) {
    eval {
        require Proc::Daemon;
        Proc::Damoen->import();
    };
    if($@) {
        warn();
    } else {
        Proc::Daemon::Init();
    }
}

my $API_URL='https://api.pinboard.in/v1/posts/add?';
my $bkmrk_url='';

# Get redirected URL's permalink
my $ua = LWP::UserAgent->new(
    requests_redirectable => [],
);

my $res = $ua->get($ARGV[0]);

if ($res->status_line == 301) {
	$bkmrk_url = $res->header( 'location');
} else {
    $bkmrk_url = $ARGV[0];
}

my $safe_bkmrk_url = uri_escape($bkmrk_url);
my $safe_title = uri_escape($ARGV[1]);
my $safe_description = uri_escape($ARGV[2]);

my $pinboard_url = $API_URL . "auth_token=$API_token&url=$safe_bkmrk_url&tags=$tag&shared=no&toread=yes&description=$safe_title&extended=$safe_description";
my $content = `curl -s \"$pinboard_url\"`;

if ($content =~ m!<result code="done" />!) {
#    print "Added to Pinboard\n";
} else {
    print STDERR " Opps $content" if $content;
    print STDERR " Bookmark not added! URL: $pinboard_url";
}
