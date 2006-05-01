#!/usr/bin/perl -w
#
# $Id$
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

package uniprot::parser;

use strict;

my $count = 0;

my $commentLine1 = "CC   -----------------------------------------------------------------------";
my $commentLine2 = "CC   Copyrighted by the UniProt Consortium, see http://www.uniprot.org/terms";
my $commentLine3 = "CC   Distributed under the Creative Commons Attribution-NoDerivs License";

our $COMPRESSION_LEVEL = 9;
our $COMPRESSION = "zlib";
our $COMPRESSION_DICTIONARY=<<END;
$commentLine1
$commentLine2
$commentLine3
$commentLine1
END

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "uniprot::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	my ($verbose, $db) = @_;
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		$doc .= $line;

		chomp($line);

		# just maybe there are records without a sequence???
		if ($line eq '//')
		{
			$m->Store($doc);
			$m->FlushDocument;
	
			$id = undef;
			$doc = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}/o)
		{
			my $fld = $1;

			if ($line =~ /^ID +(\S+)/o)
			{
				$id = $1;
				$m->IndexValue('id', $id);
			}
			elsif ($fld eq 'SQ')
			{
				if ($line =~ /SEQUENCE\s+(\d+) AA;\s+(\d+) MW;\s+([0-9A-F]{16}) CRC64;/o)
				{
					$m->IndexNumber('length', $1);
					$m->IndexNumber('mw', $2);
					$m->IndexWord('crc64', $3);
				}
				
				my $sequence = "";
				
				while ($line = <IN>)
				{
					$doc .= $line;
					chomp($line);

					last if $line eq '//';
					
					$line =~ s/\s//g;
					$sequence .= $line;
				}

				$m->Store($doc);
				$m->AddSequence($sequence);
				$m->FlushDocument;
	
				$id = undef;
				$doc = undef;
			}
			elsif (substr($fld, 0, 1) eq 'R')
			{
				$m->IndexTextAndNumbers('ref', substr($line, 5));
			}
			elsif ($fld ne 'CC')
			{
				if ($line ne $commentLine1 and
					$line ne $commentLine2 and
					$line ne $commentLine3)
				{
					$m->IndexTextAndNumbers('cc', substr($line, 5));
				}
			}
			elsif ($fld ne 'XX')
			{
				$m->IndexTextAndNumbers(lc($fld), substr($line, 5));
			}
		}
	}
}

sub version
{
	my ($self, $raw_dir, $db) = @_;
	
	$raw_dir =~ s'(sprot|trembl)/?$'uniprot';

	open RELDATE, "<$raw_dir/reldate.txt";
	
	my $vers;
	
	while (my $line = <RELDATE>)
	{
		if ($db eq 'sprot' and $line =~ /Swiss-Prot/) {
			$vers = $line;
			last;
		}
		elsif ($db eq 'trembl' and $line =~ /TrEMBL/) {
			$vers = $line;
			last;
		}
		elsif ($db eq 'sp100' and $line =~ /Swiss-Prot/) {
			$vers = $line;
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
	
	$raw_dir =~ s'(sprot|trembl)/?$'uniprot';

	if ($db eq 'sprot') {
		return "gzcat $raw_dir/uniprot_sprot.dat.gz|";
	}
	elsif ($db eq 'trembl') {
		return "gzcat $raw_dir/uniprot_trembl.dat.gz|";
	}
	elsif ($db eq 'sp100') {
		return "$raw_dir/sprot.dat";
	}
	elsif ($db eq 'sp200') {
		return "gzcat $raw_dir/sp200.dat.gz|";
	}
	elsif ($db eq 'uniprot') {
		return ( "gzcat $raw_dir/uniprot_trembl.dat.gz|", "gzcat $raw_dir/uniprot_sprot.dat.gz|");
	}
	else {
		die "unknown db: $db\n";
	}
		
#	opendir DIR, $raw_dir;
#	my @result = grep { -e "$raw_dir/$_" and $_ =~ /\.dat\.gz$/ } readdir DIR;
#	closedir DIR;
#    
#	return map { "gzcat $raw_dir/$_ |" } @result;
}

1;
