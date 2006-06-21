#!/opt/local/bin/perl -w
#
# $Id: Format_pdb.pm,v 1.1 2005/08/30 06:49:58 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_pdbfinder2;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'pdbfinder2',
		@_
	};
	my $result = bless $self, "Format_pdbfinder2";
	return $result;
}

sub pp
{
	my ($this, $q, $text, $id) = @_;
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	
	if ($text =~ /^Header\s+:\s*(.+)/mo)
	{
		$desc = lc($1);
	}
	
	return $desc;
}

1;
