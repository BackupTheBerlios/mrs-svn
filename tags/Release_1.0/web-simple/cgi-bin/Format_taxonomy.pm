#!/opt/local/bin/perl -w
#
# $Id: Format_taxonomy.pm,v 1.3 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_taxonomy;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'taxonomy',
		@_
	};
	my $result = bless $self, "Format_taxonomy";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1}) . "?db=taxonomy&id=";
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(\S+)|<a href='$&'>$&</a>|g;

	$text =~ s|^(PARENT ID\s+:\s+)(\d+)|$1<a href=$url$2>$2</a>|mo;
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	
	my ($common_name, $scientific_name);
	
	if ($text =~ /^SCIENTIFIC NAME\s+:\s+(.+)/mo)
	{
		$scientific_name = $1;
	}

	if ($text =~ /^COMMON NAME\s+:\s+(.+)/mo)
	{
		$common_name = $1;
	}
	
	if (defined $scientific_name and defined $common_name)
	{
		$desc = $q->em($scientific_name) . " ($common_name)";
	}
	else
	{
		$desc = $q->em($scientific_name);
	}
	
	return $desc;
}

1;