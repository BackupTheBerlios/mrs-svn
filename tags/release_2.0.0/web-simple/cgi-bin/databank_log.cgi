#!/usr/bin/perl

# A simple perl script to create an overview of the status of the available databanks
# Created by Maarten L. Hekkelman @ CMBI 

use CGI qw/:standard/;
use strict;

my ($db_name, $log_file, $title, $status_dir);

$db_name = param('name');
$status_dir = "/usr/data/status/";

if ($db_name eq "srs") {
	$log_file = "srs.log";
	$title = "SRS indexing log";
}
else {
	if (param('type') eq "fetch") {
		$log_file = "$db_name.mirror_log";
		$title = "Fetch log for $db_name";
	}
	elsif (param('type') eq "valid") {
		$log_file = "$db_name.validate_log";
		$title = "Validate log for $db_name";
	}
	else {
		$log_file = "$db_name.make_log";
		$title = "Make log for $db_name";
	}
}

my $log;
	
open LOG, "$status_dir/$log_file" or die "Could not open logfile";
while (<LOG>) {
	s/&/&amp;/;
	s/</&lt;/;
	s/>/&gt;/;
	$log .= $_;
}
close LOG;

print
	header, start_html(-title=>$title),
	h3($title),
	pre($log),
	end_html;

exit;
