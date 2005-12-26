#!/opt/local/bin/perl -w
#
# $Id: Format_hssp.pm,v 1.3 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_hssp;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'hssp',
		@_
	};
	my $result = bless $self, "Format_hssp";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url1 = $q->url({-full=>1}) . "?db=uniprot&id=";
	my $url2 = $q->url({-full=>1}) . "?db=pdb&id=";
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(\S+)|<a href='$&'>$&</a>|g;

	$text =~ s
	{
		^(\s+\d+\s:\s)(\S+)(\s+)(\d\w\w\w)?(?=\s+\d)
	}
	{
		if (defined $4 and length($4))
		{
			"$1" . $q->a({-href=>"$url1$2"}, $2) . $3 . 
				$q->a({-href=>"$url2$4"}, $4);
		}
		else {
			"$1" . $q->a({-href=>"$url1$2"}, $2) . $3;
		}
	}mxge;
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	
	if ($text =~ /^HEADER\s+(.+)/mo)
	{
		$desc = lc($1);
	}
	
	return $desc;
}

1;