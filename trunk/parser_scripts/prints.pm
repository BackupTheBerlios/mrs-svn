#!/usr/bin/perl -w
#
# $Id: prints.pm 79 2006-05-30 08:36:36Z maarten $
#
# Copyright (c) 2005
#      CMBI, Radboud University Nijmegen. All rights reserved.
#
# This code is derived from software contributed by Maarten L. Hekkelman
# and Hekkelman Programmatuur b.v.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the Radboud University
#        Nijmegen and its contributors.
# 4. Neither the name of Radboud University nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

package MRS::Script::prints;

our @ISA = "MRS::Script";

use strict;
use Data::Dumper;

my $count = 0;

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "MRS::Script::prints";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	my ($verbose, $db) = @_;
	
	my %ix = map { $_ => 1 } ( 'gc', 'gn', 'ga', 'gp', 'gr', 'gd', 'tp' );
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	
	while (1)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;

		if (not defined $lookahead or $lookahead =~ /^gc; /)
		{
			$m->Store($doc);
			$m->FlushDocument;
			
			$doc = undef;
			
			last unless defined $lookahead;
		}
		
		chomp($line);

		my ($fld, $text) = split(m/; */, $line, 2);
		next unless defined $text and length($text);

		if ($fld eq 'gx')
		{
			$m->IndexValue('id', $text);
		}
		elsif ($ix{$fld})
		{
			$m->IndexText($fld, $text);
		}
	}
}

sub version
{
	my ($self, $raw_dir, $db) = @_;
	
	$raw_dir =~ s'(sprot|trembl)/?$'prints';

	open RELDATE, "gzcat $raw_dir/newpr.lis.gz|";
	
	my $vers;
	
	while (my $line = <RELDATE>)
	{
		chomp($line);
		
		if ($line =~ /PRINTS VERSION \d+\.\d+/)
		{
			$vers = $line;
			$vers =~ s/^\s+//;
			last;
		}
	}

	die "Unknown db: $db" unless defined $vers;

	chomp($vers);

	return $vers;
}

sub raw_files
{
	my ($self, $raw_dir, $db) = @_;
	
	return "gzcat $raw_dir/prints*.dat.gz|";
}

# formatting

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
