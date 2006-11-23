# $Id: Format_pfam.pm,v 1.5 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_pfam;

use Data::Dumper;

our @ISA = "Format";

my @links = (
	{
		match	=> qr|^(#=GF DR\s+PFAMA;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=pfama&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PFAMB;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=pfamb&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PDB;\s)(\S+)|,
		result	=> '$1.$q->a({-href=>"$url?db=pdb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(#=GS .+AC )(\S+)|,
		result	=> '$1.$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PROSITE;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=prosite_doc&id=$2"}, $2)'
	},
);


sub new
{
	my $invocant = shift;
	my $self = {
		name => 'pfam',
		@_
	};
	my $result = bless $self, "Format_pfam";
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

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc;

	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 5, 2) eq 'DE')
		{
			$desc = substr($line, 10);
			last;
		}
	}
	
	return $desc;
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
