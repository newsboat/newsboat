#!/usr/bin/perl

use strict;
use Net::Delicious;
use LWP::UserAgent;

my $url='';
my $tag='newsbeuter';


#Get redirected URL's permalink
my $ua = LWP::UserAgent->new(
    requests_redirectable => [],
);
my $res = $ua->get($ARGV[0]);
if ($res->status_line == 301)
	{$url = $res->header( 'location');}
else
	{$url = $ARGV[0];}

my $pin = Net::Delicious->new({
	user => "change",
        pswd => "changeme",
	endpoint => "https://api.pinboard.in/v1/",
});
	
my $result = $pin->add_post({
	url   => $url,
	description => $ARGV[1],
	tags => $tag,
});
	
if (!$result) {
	# Must be an error. But who cares!
}

