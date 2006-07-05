# $Id: Format_prosite.pm,v 1.1 2005/08/30 06:49:58 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_prosite;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'prosite',
		@_
	};
	my $result = bless $self, "Format_prosite";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	my $result;
	
	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'DR')
		{
			$line =~ s{
						(\S+)(,\s+)(\S+)(?=\s*,\s+.;)
					}
					{
						"<a href='$url?db=sprot%2Btrembl\&query=ac:$1'>$1</a>" .
						"$2" .
						"<a href='$url?db=sprot%2Btrembl\&id=$3'>$3</a>"
					}xge;
		}
		elsif (substr($line, 0, 2) eq 'DO')
		{
			$line =~ s{
						(PDOC\d+)
					}
					{
						$q->a({-href=>"$url?db=prosite_doc&id=$1"}, $1)
					}xge;
		}
		
		$result .= "$line\n";
	}
	
	return $q->pre($result);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	my $state = 0;

	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'DE')
		{
			$state = 1;
			$desc .= substr($line, 5);
		}
		elsif ($state == 1)
		{
			last;
		}
	}
	
	return $desc;
}

sub to_fasta
{
	my ($this, $q, $text) = @_;
	
	my ($id, $seq, $state);
	
	$state = 0;
	$seq = "";
	
	foreach my $line (split(m/\n/, $text))
	{
		if ($state == 0 and $line =~ /^ID\s+(\S+)/)
		{
			$id = $1;
			$state = 1;
		}
		elsif ($state == 1 and substr($line, 0, 2) eq 'SQ')
		{
			$state = 2;
		}
		elsif ($state == 2 and substr($line, 0, 2) ne '//')
		{
			$line =~ s/\s+//g;
			$seq .= $line;
		}
	}
	
	$seq =~ s/(.{60})/$1\n/g;
	
	return $q->pre(">$id\n$seq\n");
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'id' => 'Identification',
		'ac' => 'Accession number',
		'cc' => 'Comments and Notes',
		'dt' => 'Date',
		'de' => 'Description',
		'gn' => 'Gene name',
		'os' => 'Organism species',
		'og' => 'Organelle',
		'oc' => 'Organism classification',
		'ox' => 'Taxonomy cross-reference',
		'ref' => 'Any reference field',
		'dr' => 'Database cross-reference',
		'kw' => 'Keywords',
		'ft' => 'Feature table data',
		'sv' => 'Sequence version',
		'fh' => 'Feature table header'
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
