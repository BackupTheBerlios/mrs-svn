#!/opt/local/bin/perl -w
#
# $Id: Format_genbank.pm,v 1.4 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_genbank;

use Data::Dumper;

our @ISA = "Format";

my @links = (
	{
		match	=> qr|^(DBSOURCE\s+REFSEQ:\s+accession\s+)([A-Z0-9_]+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=refseq_release|refseq_updates&id=$2"}, $2)'
	},
	{
		match	=> qr|(/db_xref="embl:)(\S+)(?=[;"])|i,
		result	=> '$1.$q->a({-href=>"$url?db=embl&query=acc:$2"}, $2)'
	},
	{
		match	=> qr[(/db_xref="(swiss-prot|sptrembl|uniprot/swiss-prot|uniprot/trembl):)(\S+)(?=[;"])]i,
		result	=> '$1.$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$3"}, $3)'
	},
	{
		match	=> qr|(/db_xref="taxon:)(\S+)(?=[;"])|i,
		result	=> '$1.$q->a({-href=>"$url?db=taxonomy&id=$2"}, $2)'
	},
);


sub new
{
	my $invocant = shift;
	my $self = {
		name => 'genbank',
		@_
	};
	my $result = bless $self, "Format_genbank";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	$text = $this->link_url($text);
	
	foreach my $l (@links)
	{
		$text =~ s/$l->{match}/$l->{result}/eegm;
	}
	
	return $q->pre($text);
}

sub to_fasta
{
	my ($this, $q, $text) = @_;
	
	my ($id, $seq, $state);
	
	$state = 0;
	$seq = "";
	
	foreach my $line (split(m/\n/, $text))
	{
		if ($state == 0 and $line =~ /^ACCESSION\s+(\S+)/)
		{
			$id = $1;
			$state = 1;
		}
		elsif ($state == 1 and substr($line, 0, 6) eq 'ORIGIN')
		{
			$state = 2;
		}
		elsif ($state == 2 and substr($line, 0, 2) ne '//')
		{
			$line =~ s/^\s*\d+\s+//g;
			$line =~ s/\s+//g;
			$seq .= $line;
		}
	}
	
	$seq =~ s/(.{60})/$1\n/g;
	
	return $q->pre(">$id\n$seq\n");
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	my $state = 0;

	foreach my $line (split(m/\n/, $text))
	{
		if ($state == 0 and substr($line, 0, 10) eq 'DEFINITION')
		{
			$state = 1;
			$desc .= substr($line, 12);
		}
		elsif ($state == 1 and substr($line, 0, 12) eq '            ')
		{
			$desc .= ' ' . substr($line, 12);
		}
		elsif ($state == 1)
		{
			last;
		}
	}
	
	return $desc;
}


1;
