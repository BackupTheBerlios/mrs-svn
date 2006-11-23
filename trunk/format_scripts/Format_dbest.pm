# $Id: Format_dbest.pm,v 1.3 2005/08/26 09:08:54 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_dbest;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'dbest',
		@_
	};
	my $result = bless $self, "Format_dbest";
	return $result;
}

#sub pp
#{
#	my ($this, $q, $text) = @_;
#	
#	my $url = $q->url({-full=>1});
#	
#	$text = $this->link_url($text);
#	
#	foreach my $l (@links)
#	{
#		$text =~ s/$l->{match}/$l->{result}/eegm;
#	}
#	
#	return $q->pre($text);
#}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $est = "";
	my $os = "";
	my $lib = "";
	my $done = 0;

	foreach my $line (split(m/\n/, $text))
	{
		if ($line =~ /EST name:\s+(.+)/)
		{
			$est = $1;
			++$done;
		}
		elsif ($line =~ /Organism:\s+(.+)/)
		{
			$os = $1;
			++$done;
		}
		elsif ($line =~ /Lib Name:\s+(.+)/)
		{
			$lib = $1;
			++$done;
		}
		
		last if $done == 3;
	}
	
	$os = $q->em($os);
	return "$est $os $lib";
}

1;
