#!perl

package Format;

sub link_url
{
	my ($this, $text) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(([-\$_.A-Za-z0-9!*'(),/?=:&;[\]\|+]+\|%[0-9A-Fa-f]{2})+)|<a href='$&'>$&</a>|g;
	
	return $text;
}

package Embed::WSFormat;

use strict;
use CGI;

my $q = new CGI;
my %formatters;

sub getFormatter
{
	my $name = shift;
	
	if (not defined $formatters{$name})
	{
		require ("../web-simple/cgi-bin/Format_${name}.pm");
		
		$formatters{$name} = "Format_$name"->new;
	}
	
	return $formatters{$name};
};

sub pretty
{
	my ($name, $text, $id) = @_;
	
	my $result;
	
	eval
	{
		my $url = undef;

		my $fmt = &getFormatter($name);

		$result = $fmt->pp($q, $text, $id, $url);
	};
	
	if ($@)
	{
		$result = "Error in formatting entry: $@";
	}
	
	return $result;
}

1;
