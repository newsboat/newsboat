#!/usr/bin/perl

use strict;
use LWP::UserAgent;
use URI::Escape;

# Daemonising the bookmarking process
# Use this only is you are sure things 
# are working as expected
# set to '0' if not needed
my $daemon = 1;

if ($daemon){
    eval{
        require Proc::Daemon;
        Proc::Damoen->import();
    };
    unless ($@){
        Proc::Daemon::Init();
    }
}

my $API_URL='https://api.pinboard.in/v1/posts/add?';
my $bkmrk_url='';
my $tag='newsbeuter';
my $API_token='***REPLACE***';  # Get yours at https://pinboard.in/settings/password
                                # Of the form 'username:alphanumeric'

# Get redirected URL's permalink
my $ua = LWP::UserAgent->new(
    requests_redirectable => [],
);

# $ARGV[0] is the URL of the article
# $ARGV[1] is the title of the article
my $res = $ua->get($ARGV[0]);

if ($res->status_line == 301){
	$bkmrk_url = $res->header( 'location');
}else{
    $bkmrk_url = $ARGV[0];
}

my $safe_desc=uri_escape($ARGV[1]);
my $pinboard_url = $API_URL . "url=$bkmrk_url&tags=$tag&shared=no&toread=yes&description=$safe_desc&auth_token=$API_token";
my $content = `curl -s \"$pinboard_url\"`;

if ($content =~ m!<result code="done" />!){
#    print "Added to Pinboard\n";
}else{
    print "Something went wrong, not added. ";
    print "See response: $content" if $content;
}
