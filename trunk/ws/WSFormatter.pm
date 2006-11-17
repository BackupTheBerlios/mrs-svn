#!perl

package Format;

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'default',
		@_
	};
	my $result = bless $self, "Format";
	return $result;
}

sub link_url
{
	my ($this, $text) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(([-\$_.A-Za-z0-9!*'(),/?=:&;[\]\|+]+\|%[0-9A-Fa-f]{2})+)|<a href='$&'>$&</a>|g;
	
	return $text;
}

sub pp
{
	my ($this, $q, $text, $id) = @_;
	
	$text = $this->link_url($text);
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my @lines = split(m/\n/, $text);
	splice(@lines, 7);
	push @lines, "...";
	$text = join("\n", @lines);
	
	return $q->pre($text);
}

package Embed::WSFormat;

use strict;
use CGI;

my $q = new CGI;
my %formatters;

sub getFormatter
{
	my $name = shift;
	
	if ($name eq 'default')
	{
		$formatters{$name} = Format->new unless defined $formatters{$name};
	}
	else
	{
		my $plugin = "../web-simple/cgi-bin/Format_${name}.pm";
		my $mtime = -M $plugin;
		
		if (not defined $formatters{$name} or $formatters{$name}{mtime} != $mtime )
		{
	        local *FH;
	        open FH, $plugin or die "open '$plugin' $!";
	        local($/) = undef;
	        my $sub = <FH>;
	        close FH;
	
            eval $sub;
	        die $@ if $@;
	
	        #cache it unless we're cleaning out each time
			$formatters{$name} = "Format_$name"->new;
	        $formatters{$name}{mtime} = $mtime;
		}
	}
	
	return $formatters{$name};
};

sub html
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

sub title
{
	my ($name, $text, $id) = @_;
	
	my $result;

	eval
	{
		my $url = undef;

		my $fmt = &getFormatter($name);

		$result = $fmt->describe($q, $text, $id, $url);
	};
	
	if ($@)
	{
		$result = "Error in formatting entry: $@";
	}
	
	return $result;
}

1;
