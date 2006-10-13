#!/opt/local/bin/perl -w
#
# $Id: Format_prints.pm,v 1.18 2005/09/10 07:14:05 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_prints;

use Data::Dumper;
use POSIX qw(strftime);
use Time::Local;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'prints',
		@_
	};
	my $result = bless $self, "Format_prints";
	return $result;
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	if (not defined $url or length($url) == 0) {
		my $url = $q->url({-full=>1});
	}
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	my %links;
	
	open TEXT, "<", \$text;
	while (my $line = <TEXT>)
	{
		if ($line =~ m/^(tp|st);\s*(.+)/)
		{
			foreach my $id (split(m/\s+/, $2))
			{
				$links{$id} = $q->a({-href=>"$url?db=sprot%2Btrembl&query=id:$id%20OR%20ac:$id"}, $id);
			}
		}
	}
	close TEXT;
	
	foreach my $id (keys %links)
	{
		$text =~ s/\b$id\b/$links{$id}/g;
	}
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	my $state = 0;

	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 3) eq 'gt;')
		{
			$state = 1;
			$desc .= substr($line, 3);
		}
		elsif ($state == 1)
		{
			last;
		}
	}
	
	return $desc;
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'gd' => 'Description',
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
