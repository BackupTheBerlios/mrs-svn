#!/usr/bin/perl -w

# A simple perl script to create an overview of the status of the available databanks
# Created by Maarten L. Hekkelman @ CMBI 

use MRSCGI;
use CGI::Carp qw(fatalsToBrowser);
use strict;
use POSIX qw(strftime);
use File::stat;
use Data::Dumper;

my $status_dir = "/Users/maarten/data/status/";
my $q = new MRSCGI(script => 'databanks.cgi');

&main();
exit;

sub reset() {
	my $db = shift;
	
	my %failed_dbs;

	open FAILED, "<$status_dir/failed_dbs.txt";
	while (my $line = <FAILED>) { chomp($line); $failed_dbs{$line} = 1; }
	close FAILED;
	
	open FAILED, ">$status_dir/failed_dbs.txt" or die "Could not open failed_dbs.txt for edit\n";
	foreach my $f (keys %failed_dbs) {
		next if $f eq $db;
		print FAILED "$f\n";
	}
	close FAILED;
}

sub log() {
	my ($db, $type, $nr) = @_;
	my ($log_file, $title);
	
	if ($type eq "fetch") {
		$log_file = "$db.mirror_log";
		$title = "Fetch log for $db";
	}
	else {
		$log_file = "$db.make_log";
		$title = "Make log for $db";
	}
	
	$log_file = "$log_file.$nr" if ($nr);
	
	my $log;
		
	open LOG, "$status_dir/$log_file" or die "Could not open logfile";
	while (<LOG>) {
		s/&/&amp;/;
		s/</&lt;/;
		s/>/&gt;/;
		$log .= $_;
	}
	close LOG;

	my ($link_older, $link_newer);
	
	if (not defined $nr or $nr < 3) {
		$link_older = $q->a({-href=>sprintf($q->base_url . "/cgi-bin/databanks.cgi?log=$db&type=$type&nr=%d", $nr + 1)}, "next");
	}

	if ($nr) {
		$link_newer = $q->a({-href=>sprintf($q->base_url . "/cgi-bin/databanks.cgi?log=$db&type=$type&nr=%d", $nr - 1)}, "previous");
	}
	else {
		$link_newer = "previous";
	}
	
	return $q->span(
		$q->h3($title),
		$link_newer,
		$link_older ? $link_older : "next",
		$q->pre(
			$log
		)
	);
}

sub list() {
	my @rows;
	my %failed_dbs;
	
	open FAILED, "<$status_dir/failed_dbs.txt";
	while (my $line = <FAILED>) { chomp($line); $failed_dbs{$line} = 1; }
	close FAILED;
	
	push @rows, $q->Tr(
			$q->th({-width=>'20%', -colspan=>2}, $q->a({-href=>'./databanks.cgi?order=name'}, $q->b("name"))),
			$q->th({-width=>'40%', -colspan=>2}, $q->a({-href=>'./databanks.cgi?order=mirror'}, $q->b("last mirror"))),
			$q->th({-width=>'40%', -colspan=>2}, $q->a({-href=>'./databanks.cgi?order=make'}, $q->b("last make"))),
		);
	
	my ($result, @files);
	
	opendir(DIR, $status_dir) or die "can't opendir $status_dir: $!";
	foreach my $f (sort grep { m/\.mirror_log$/ and -f "$status_dir/$_" } readdir(DIR))
	{
		$f =~ s/\.mirror_log//;
		push @files, $f;
	}
	closedir DIR;
	
	if ($q->param('order') and ($q->param('order') eq "mirror" or $q->param('order') eq "make"))
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

		$fetch_log_date		= 0;
		$fetch_done_date	= 0;
		$make_log_date		= 0;
		$make_done_date		= 0;
	
		eval
		{
			$fetch_log_date		= stat("$status_dir/$file.mirror_log")->mtime;
			$fetch_done_date	= stat("$status_dir/$file.fetched")->mtime;
			$make_log_date		= stat("$status_dir/$file.make_log")->mtime;
			$make_done_date		= stat("$status_dir/$file.make")->mtime;
		};
		
		push @rows, $q->Tr(
			$q->td({-class=>$failed_dbs{$file} ? "failed" : "ok"}, $file),
			$q->td({-class=>$failed_dbs{$file} ? "failed" : "ok"}, 
				$failed_dbs{$file} ? $q->a({-href=>"./databanks.cgi?reset=$file"}, "reset") : "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
			),
			$q->td({-class=>($fetch_log_date <= $fetch_done_date ? "ok" : "failed")},
				strftime("%d-%b-%Y, %H:%M:%S", localtime($fetch_log_date))),
			$q->td({-class=>($fetch_log_date <= $fetch_done_date ? "ok" : "failed")},
				$q->a({-href=>"./databanks.cgi?log=$file&type=fetch"}, "log")
			),
			$q->td({-class=>($make_log_date <= $make_done_date ? "ok" : "failed")},
				strftime("%d-%b-%Y, %H:%M:%S", localtime($make_log_date))),
			$q->td({-class=>($make_log_date <= $make_done_date ? "ok" : "failed")},
				$q->a({-href=>"./databanks.cgi?log=$file&type=make"}, "log")
			),
		);
	}
	
	return $q->div({-class=>'list'},
				$q->table({-cellspacing=>'0', -cellpadding=>'0'},
					$q->caption("Status for $ENV{'SERVER_NAME'} at ".strftime("%d-%b-%Y, %H:%M:%S", localtime)),
					@rows
				)
			);
}

sub main()
{
	if ($q->param('reset')) {
		&reset($q->param('reset'));
		print $q->redirect($q->base_url . "/cgi-bin/databanks.cgi");
		exit;
	}
	

	my %html_options = (
		-title=>"databanks @ $ENV{SERVER_NAME}",
		-script=>{'src'=>$q->base_url . "/mrs.js"},
		-style=>{'src'=>$q->base_url . "/mrs_style.css"}
	);
	
	my $text;
	
	if ($q->param('log')) {
		$text = &log($q->param('log'), $q->param('type'), $q->param('nr'));
	}
	else {
		$text = &list();
	}
	
	my $update_active = "No update running at this time";
	if (-f "$status_dir/UPDATE_LOCK") {
		$update_active = "Automatic update is currently running";
	}
	
	print
		$q->header(-Refresh=>'5', -charset=>'utf-8'),
		$q->start_html(%html_options),
		$q->menu(),
		'<div id="main">',
			$q->h4($update_active),
			$text,
		'</div>',
		$q->end_html;
	
}


