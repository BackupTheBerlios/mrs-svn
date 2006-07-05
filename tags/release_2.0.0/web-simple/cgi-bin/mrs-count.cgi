#!/usr/bin/perl
#
# $Id: mrs-count.cgi,v 1.1 2005/10/13 14:14:07 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use CGI;
use MRS;

my $db;
my @fields;
my %fldLabels;
my $exact = 0; # make this an option

our @dbs;
our $mrs_data;
our $mrs_script;

my $settings = 'mrs.conf';
unless (my $return = do $settings)
{
	warn "couldn't parse $settings: $@" if $@;
	warn "couldn't do $settings: $!" unless defined $return;
	warn "couldn't run $settings" unless $return;
}

my @dbIds = map { $_->{id} } @dbs;
my %dbLabels = map { $_->{id} => $_->{name} } @dbs;
my %dbFilters = map { $_->{id} => $_->{filter} } @dbs;
my %dbInfo = map { $_->{id} => $_ } @dbs;

foreach my $dbid (@dbIds)
{
	if ($dbid =~ /\|/)
	{
		foreach my $i (split(m/\|/, $dbid))
		{
			$dbFilters{$i} = $dbFilters{$dbid};
		}
	}
}

# make sure the plugin knows where to find the mrs files

$ENV{MRS_DATA_DIR} = $mrs_data;

my $q = new CGI;

&main();
exit;

sub main()
{
	my ($db, $query, $wildcard) = ($q->param('db'), $q->param('q'), $q->param('w'));
	
	$wildcard = 1 unless defined $wildcard;
	
	die "no db or query" unless defined $db and defined $query;
	
	my $m = new MRS::MDatabank($db);
	die "no such db" unless defined $m;
	
	my $cnt = 0;
	
	eval {
		my $i = $m->Find($query, $wildcard ? 1 : 0);
		$cnt = $i->Count(0);
	};
	
	my $reply=<<END;
Content-Type: text/xml

<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<response>
  <result>$cnt</result>
</response>
END

	print $reply;
}
