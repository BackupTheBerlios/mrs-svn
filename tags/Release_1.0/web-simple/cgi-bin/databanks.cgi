#!/usr/pkg/bin/perl -w

# A simple perl script to create an overview of the status of the available databanks
# Created by Maarten L. Hekkelman @ CMBI 

use CGI;
use CGI::Carp qw(fatalsToBrowser);
use strict;
use POSIX qw(strftime);
use File::stat;
use Data::Dumper;

my $status_dir = "/usr/data/status/";
my $q = new CGI;
my @rows;
my %failed_dbs;

open FAILED, "<$status_dir/failed_dbs.txt";
while (my $line = <FAILED>) { chomp($line); $failed_dbs{$line} = 1; }
close FAILED;

push @rows, $q->Tr(
		$q->th({-bgcolor=>'#aaaaff', -width=>'20%'}, $q->a({-href=>'./databanks.cgi?order=name'}, $q->b("name"))),
		$q->th({-bgcolor=>'#aaaaff', -width=>'40%'}, $q->a({-href=>'./databanks.cgi?order=mirror'}, $q->b("last mirror"))),
		$q->th({-bgcolor=>'#aaaaff', -width=>'40%'}, $q->a({-href=>'./databanks.cgi?order=make'}, $q->b("last make"))),
	);

my ($result, @files);

opendir(DIR, $status_dir) or die "can't opendir $status_dir: $!";
foreach my $f (sort grep { m/\.mirror_log$/ and -f "$status_dir/$_" } readdir(DIR))
{
	$f =~ s/\.mirror_log//;
	push @files, $f;
}
closedir DIR;

if ($q->param('order') eq "mirror" or $q->param('order') eq "make")
{
	my %sorted_files;
	foreach my $file (@files)
	{
		my $key;

		if ($q->param('order') eq "mirror")
		{
			my $stat = stat("$status_dir/$file.mirror_log");
			$key = strftime("%Y%m%d%H%M%S$file", localtime($stat->mtime)) if $stat;
		}
		elsif ($q->param('order') eq "make")
		{
			my $stat = stat("$status_dir/$file.make_log");
			$key = strftime("%Y%m%d%H%M%S$file", localtime($stat->mtime)) if $stat;
		}

		$sorted_files{ $key } = $file;
	}
	
	undef @files;
	
	for my $file (reverse sort keys %sorted_files)
	{
		push @files, $sorted_files{$file};
	}
}

foreach my $file (@files)
{
	my ($fetch_log_date, $fetch_done_date, $make_log_date, $make_done_date);

	eval
	{
		$fetch_log_date		= stat("$status_dir/$file.mirror_log")->mtime;
		$fetch_done_date	= stat("$status_dir/$file.fetched")->mtime;
		$make_log_date		= stat("$status_dir/$file.make_log")->mtime;
		$make_done_date		= stat("$status_dir/$file.make")->mtime;
	};
	
	push @rows, $q->Tr(
		$q->td({-bgcolor=>$failed_dbs{$file} ? "\#ffaaaa" : "\#ffffaa"}, $file),
		$q->td({-bgcolor=> $fetch_log_date <= $fetch_done_date ? "\#aaffaa" : "\#ffaaaa"},
			strftime("%d-%b-%Y, %H:%M:%S", localtime($fetch_log_date)),
			'&nbsp;&nbsp;',
			$q->a({-href=>"./databank_log.cgi?name=$file&type=fetch"}, "log")
		),
		$q->td({-bgcolor=> $make_log_date <= $make_done_date ? "\#aaffaa" : "\#ffaaaa"},
			strftime("%d-%b-%Y, %H:%M:%S", localtime($make_log_date)),
			'&nbsp;&nbsp;',
			$q->a({-href=>"./databank_log.cgi?name=$file&type=make"}, "log")
		),
	);
}

print
	$q->header(-Refresh=>'10'),
	$q->start_html(-title=>"databanks @ $ENV{SERVER_NAME}"),
	$q->h3("Status for $ENV{'SERVER_NAME'} at ".strftime("%d-%b-%Y, %H:%M:%S", localtime)),
	$q->table({-width=>'100%'}, @rows),
	$q->end_html;

exit;
