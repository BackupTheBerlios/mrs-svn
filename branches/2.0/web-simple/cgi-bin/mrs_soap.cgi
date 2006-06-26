#!/usr/bin/perl
#
# $Id: mrs_soap.cgi,v 1.9 2005/10/11 13:17:50 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

package mrs;

use vars qw(@ISA);
@ISA = qw(SOAP::Server::Parameters);

use SOAP::Transport::HTTP;
use Data::Dumper;
use MRS;
use CGI;
use Format;

SOAP::Transport::HTTP::CGI
	-> dispatch_to('mrs')
	-> handle;

our @dbs;
my %dbLabels;
my %dbFilters;
my @dbIds;

my $ns = "http://mrs.cmbi.ru.nl/mrs";

sub init()
{
	my $settings = 'mrs.conf';
	unless (my $return = do $settings)
	{
		warn "couldn't parse $settings: $@" if $@;
		warn "couldn't do $settings: $!" unless defined $return;
		warn "couldn't run $settings" unless $return;
	}
	%dbLabels = map { $_->{id} => $_->{name} } @dbs;
	%dbFilters = map { $_->{id} => $_->{filter} } @dbs;
	@dbIds = map { $_->{id} } @dbs;
	
	$ENV{MRS_DATA_DIR} = $mrs_data;
}

my $db;

sub entry()
{
	my ($class, $db, $id) = @_;
	
	init();
	
	my @ids;
	
	if (ref($id) eq "array")
	{
		push @ids, @{$id};
	}
	else
	{
		push @ids, $id;
	}

	my @recs;
	
	my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
	
	foreach my $i (@ids)
	{
		push @recs, $d->Get($i);
	}
	
	if (ref($id) eq "array")
	{
		return SOAP::Data->name('result')
						 ->type('array')
						 ->uri('http://cmbidb1.cmbi.kun.nl/')
						 ->value(\@recs);
	}
	else
	{
		my $result = $recs[0];
		return SOAP::Data->name('result')
						 ->type('string')
						 ->uri('http://cmbidb1.cmbi.kun.nl/')
						 ->value($result);
	}
}

sub query()
{
	my $self = shift;
	my ($db, $query, $summary) =
		SOAP::Server::Parameters::byNameOrOrder([qw(db query summary)], @_);

	init();

	my @qq;
	
	if (ref($query) eq "array")
	{
		push @qq, @{$query};
	}
	else
	{
		push @qq, $query;
	}

	my @recs;

	my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
	
	my $cgi;
	$cgi = new CGI if defined $summary;

	foreach my $q (@qq)
	{
		my $i = $d->Find($q, 0);
		my @results;
		
		if (defined $i and $i->Count(0) > 0)
		{
			while ($r = $i->Next)
			{
				if ($summary)
				{
					my %data;
					$data{id} = $r;
					$data{summary} = Format::desc($dbFilters{$db}, $cgi, $d->Get($r));
					push @results, \%data;
				}
				else
				{
					push @results, $r;
				}
			}
		}
		
		push @recs, [@results];
	}
	
	if (ref($query) eq "array")
	{
		return SOAP::Data->name('result')
						 ->type('array')
						 ->uri('http://cmbidb1.cmbi.kun.nl/')
						 ->value(\@recs);
	}
	else
	{
		my $result = $recs[0];
		return SOAP::Data->name('result')
						 ->type('array')
						 ->uri('http://cmbidb1.cmbi.kun.nl/')
						 ->value($result);
	}
}

#sub getSequence()
#{
#	my $self = shift;
#	my ($id, $db) =
#		SOAP::Server::Parameters::byNameOrOrder([qw(id db)], @_);
#
#	init();
#
#	my @recs;
#
#	my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
#	
#	my $data = $d->Sequence($id)
#		or die "ID $id not found in databank $db";
#	
##	my @result;
##	push @result, SOAP::Data->new(name => 'id', type => 'xsd:string', attr => {}, value => $id);
##	push @result, SOAP::Data->new(name => 'data', type => 'xsd:string', attr => {}, value => $data);
##
#	return SOAP::Data->name('result')
#					 ->type('string')
#					 ->uri('http://mrs.cmbi.ru.nl/mrs')
#					 ->value($data);
#}
#
#sub searchEntries()
#{
#	my $self = shift;
#	my ($db, $query) =
#		SOAP::Server::Parameters::byNameOrOrder([qw(db query)], @_);
#
#	init();
#
#	my @recs;
#
#	my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
#	
#	my @result;
#	
#	my $i = $d->Find($query, 0)
#		or die "ID $query not found in databank $db";
#	
#	my @results;
#	
#	if (defined $i and $i->Count > 0)
#	{
#		for (;;)
#		{
#			push @results, $i->Id;
#			last unless $i->Next;
#		}
#	}
#	
##	my @result;
##	push @result, SOAP::Data->new(name => 'id', type => 'xsd:string', attr => {}, value => $id);
##	push @result, SOAP::Data->new(name => 'data', type => 'xsd:string', attr => {}, value => $data);
##
#	return SOAP::Data->name('result')
#					 ->type('array')
#					 ->uri('http://mrs.cmbi.ru.nl/mrs')
#					 ->value(\@results);
#}

sub getEntry()
{
	my $self = shift;
	my ($id, $db) =
		SOAP::Server::Parameters::byNameOrOrder([qw(id db)], @_);

	init();

	my @recs;

	my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
	
	my %result;
	$result{'id'} = $id;
	$result{'data'} = $d->Get($id)
		or die "ID $id not found in databank $db";

	return SOAP::Data->name('result')
					 ->type('DBEntry')
					 ->uri('http://cmbidb1.cmbi.kun.nl/')
					 ->value($result);
}
