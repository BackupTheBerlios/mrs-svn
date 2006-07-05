# $Id: Format_omim.pm,v 1.4 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

package Format_omim;

our @ISA = "Format";

use Data::Dumper;
use strict;

my %labels = (
	AV => 'Allelic variation',
	CD => 'Creation date',
	CN => 'Contributor name',
	CS => 'Clinical Synopsis',
	ED => 'Edit history',
	MN => 'Mini-Mim',
	NO => 'Number',
	RF => 'References',
	SA => 'See Also',
	TI => 'Title',
	TX => 'Text',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'omim',
		@_
	};
	my $result = bless $self, "Format_omim";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;

	$text = $this->link_url($text);
	
	$text =~ s/^\*RECORD\*\s+//;
	
	my @fields = split(/^\*FIELD\* /om, $text);

	my $url = $q->url({-full=>1});
	
	my @data;
	
	foreach my $f (@fields)
	{
		my $code;
		
		if ($f =~ /^(\w\w)\s/)
		{
			$code = $1;
			my $label = $code;
			$label = $labels{$label} if defined $labels{$label};
			
			push @data, $q->h3($label);
			
			$f = substr($f, 3);
		}

		if ($code eq 'CS')
		{
			my @dl;
			foreach my $p (split(/\n\n/, $f))
			{
				my ($dt, $dd) = split(/:/, $p);
				push @dl, $q->dt($q->b("$dt:")), $q->dd($dd);
			}
			push @data, $q->dl(@dl);
			next;
		}

		if ($code eq 'CN' or $code eq 'ED')
		{
			$f = join($q->br, split(/\n/, $f));
			push @data, $q->p($f);
			next;
		}

		if ($code eq 'RF' and 0)
		{
			my @dl;
			foreach my $p (split(/\n\n/, $f))
			{
				my ($dt, $dd) = split(/:/, $p);
				push @dl, $q->dt("$dt:"), $q->dd($dd);
			}
			push @data, $q->dl(@dl);
			next;
		}
		
		foreach my $p (split(/\n\n/, $f))
		{
			if ($code eq 'TX' or $code eq 'AV')
			{
				$p =~ s|\((\S+?;\s+)?(\d{6})\)|($1<a class='bluelink' href='$url?db=omim\&id=$2'>$2</a>)|g;
				$p =~ s|\((\S+?;\s+)?(\d{6})\.(\d{4})\)|($1<a class='bluelink' href='$url?db=omim\&id=$2#.$3'>$2.$3</a>)|g;
				
				$p =~ s|^[A-Z ]+$|<h4>$&</h4>|mg;
			}
			
			if ($code eq 'AV')
			{
				$p =~ s|^\.(\d{4})|<a name='.$1'>.$1</a>|mg;
			}
			
			push @data, $q->p($p);
		}
	}
	
	return join("\n", @data);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	$text =~ s/^\*RECORD\*\s+//;
	
	my @fields = split(m/^\*FIELD\* /om, $text);
	my %f = map { substr($_, 0, 2) => substr($_, 2) } @fields;
	
	my $desc = $f{TI};
	
	$desc =~ s/^\D?\d{6}\s//om;
	$desc = (split(m/;/, $desc))[0];
	
	return $desc;
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'no' => 'Number',
		'id' => 'Number',
		'ti' => 'Title',
		'mn' => 'Mini-Mim',
		'av' => 'Allelic variation',
		'tx' => 'Text',
		'sa' => 'See also',
		'rf' => 'References',
		'cs' => 'Clinical Synopsis',
		'cn' => 'Contributor name',
		'cd' => 'Creation name',
		'cd_date' => 'Creation date',
		'ed' => 'Edit history',
		'ed_date' => 'Edit history (date)',
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
