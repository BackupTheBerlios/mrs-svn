# first the package for the base object of formatters
#
# $Id: Format.pm,v 1.5 2005/09/13 07:02:40 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

package Format;

use Data::Dumper;
use strict;

my %factory;

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

sub to_fasta
{
	my ($this, $q, $text) = @_;
	
	return $q->pre('Cannot create fasta format');
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	return $id;
}

sub init
{
	$factory{default} = new Format(name => 'default');

	opendir(DIR, ".") or die "could not opendir: $!";
	foreach my $p (sort grep { m/^Format_.*\.pm$/ and -f $_ } readdir(DIR))
	{
		require "$p";

		$p =~ s/\.pm$//;
		my $f = $p->new;
		
		$factory{$f->name} = $f;
	}
	closedir(DIR);
}

sub call
{
	my $meth = shift;
	my $name = shift;

	init if scalar (keys %factory) == 0;

	my $pp = $factory{$name};
	$pp = $factory{default} unless defined $pp;
	
	return $pp->$meth(@_);
}

sub pre
{
	return &call('pp', @_)
}

sub desc
{
	return &call('describe', @_)
}

sub fasta
{
	return &call('to_fasta', @_)
}

sub field_name
{
	return &call('to_field_name', @_)
}

sub name
{
	my $this = shift;
	return $this->{name};
}

1;
