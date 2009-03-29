#!/usr/bin/perl

# Scrape IDL definitions from the HTML 5 Draft Recommendation
#
# usage: idl_scraper.pl http://www.whatwg.org/specs/web-apps/current-work/

use strict;
use warnings;

use URI;
use Web::Scraper;

my $res = scraper {
    process 'pre.idl', 'idl[]' => 'TEXT';
    result 'idl';
}->scrape(URI->new(shift));

$" = "\n\n";
print "@$res";

print "\n";
