#!/opt/local/bin/perl -w
#
# $Id: Format_go.pm,v 1.3 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_go;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'go',
		@_
	};
	my $result = bless $self, "Format_go";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1}) . "?db=go&id=";
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(\S+)|<a href='$&'>$&</a>|g;

	$text =~ s|(?<!id: )(GO:)(\d+)|<a href=$url$2>$1$2</a>|g;
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	
	if ($text =~ /^name:\s*(.+)/mo)
	{
		$desc = $1;
	}
	
	return $desc;
}

1;