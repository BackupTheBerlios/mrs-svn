#!/opt/local/bin/perl -w
#
# $Id: Format_goa.pm,v 1.4 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_goa;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'goa',
		@_
	};
	my $result = bless $self, "Format_goa";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = "<a href='" . $q->url({-full=>1}) . "?db=";
	my $result = "";
	
	foreach my $line (split(m/\n/, $text))
	{
		chomp($line);
		my @fields = split(m/\t/, $line);
	
		my @labels = (
			'DB: ',
			'DB_Object_ID: ',
			'DB_Object_Symbol: ',
			'Qualifier: ',
			'GOid: ',
			'DB:Reference: ',
			'Evidence: ',
			'With: ',
			'Aspect: ',
			'DB_Object_Name: ',
			'Synonym: ',
			'DB_Object_Type: ',
			'Taxon_ID: ',
			'Date: ',
			'Assigned_By: '
		);
	
		for (my $n = 0; $n < 15; ++$n)
		{
			my $value = $fields[$n];
	
			$value = "${url}sprot%2Btrembl&id=$value'>$value</a>" if ($n == 2);
			$value = "${url}sprot%2Btrembl&query=ac:$value'>$value</a>" if ($n == 1);
			$value =~ s|(GO:)(\d+)|${url}go\&id=$2'>$1$2</a>|g if ($n == 4);
			
			my $line = $labels[$n] . $value . "\n";
			
			$result .= $line;
		}
		
		$result .= "\n\n";
	}

	return $q->pre($result);
}

sub describe
{
	my ($this, $q, $text) = @_;

	chomp($text);
	my @fields = split(m/\t/, $text);
	
	my $desc = $fields[9];
	
	return $desc;
}

1;
