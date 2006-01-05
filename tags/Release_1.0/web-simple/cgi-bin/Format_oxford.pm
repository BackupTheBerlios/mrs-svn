#!/opt/local/bin/perl -w
#
# $Id: Format_oxford.pm,v 1.3 2005/09/20 10:40:15 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_oxford;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'oxford',
		@_
	};
	my $result = bless $self, "Format_oxford";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
#	my $url = "<a href='" . $q->url({-full=>1}) . "?db=";
	my $result;
	
	chomp($text);
	my @fields = split(m/\t/, $text);

	my @labels = (
		'ID',
		'Human EntrezGene ID',
		'Human Symbol',
		'Human Chr',
		'Mouse MGI Acc ID',
		'Mouse EntrezGene ID',
		'Mouse Symbol',
		'Mouse Chr',
		'Mouse cM',
		'Mouse Band',
		'Data Attributes',
		'Cytoloc number min',
		'Cytoloc number max'
	);
	
	my @rows;
	
	push @rows, $q->Tr(
		$q->th('Field'),
		$q->th('Value')
	);
	
	for (my $n = 0; $n < scalar @fields; ++$n)
	{
		push @rows, $q->Tr(
			$q->td($labels[$n]),
			$q->td($fields[$n])
		);
	}

	return $q->div({-class=>'list'},
		$q->table({-cellspacing=>'0', -cellpadding=>'0'}, @rows));
}

sub describe
{
	my ($this, $q, $text) = @_;

	chomp($text);
	my @fields = split(m/\t/, $text);
	
	my $desc = "Human: $fields[1]; Mouse: $fields[5]";
	
	return $desc;
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'id'		=> 'Unique ID',
		'llhid'		=> 'Human EntrezGene ID',
		'hsym'		=> 'Human Symbol',
		'loc'		=> 'Human Chr',
		'acc'		=> 'Mouse MGI Acc ID',
		'llmid'		=> 'Mouse EntrezGene ID',
		'msym'		=> 'Mouse Symbol',
		'chr'		=> 'Mouse Chr',
		'cm'		=> 'Mouse cM',
		'cb'		=> 'Mouse Band',
		'data'		=> 'Data Attributes',
		'loc_min'	=> 'Cytoloc number min',
		'loc_max'	=> 'Cytoloc number max'
	);
	
	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
