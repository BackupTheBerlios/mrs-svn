# MRS plugin for creating an EMBL db
#
# $Id: embl.pm 131 2006-08-10 12:02:09Z hekkel $
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

package embl::parser;

use strict;

my $count = 0;

our $COMPRESSION_LEVEL = 9;
our $COMPRESSION = "zlib";

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "embl::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		$doc .= $line;

		chomp($line);

		if ($line eq '//')
		{
			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}(.+)/o)
		{
			my $fld = $1;
			my $text = $2;

			if ($fld eq 'ID')
			{
				my @flds = split(m/;\s*/, $text);

				$m->IndexValue('id', $flds[0]);
				$flds[1] =~ m/SV (\d+)/ && $m->IndexNumber('sv', $1);
				$m->IndexWord('topology', lc($flds[2]));
				$m->IndexWord('mt', lc($flds[3]));
				$m->IndexWord('dc', lc($flds[4]));
				$m->IndexWord('td', lc($flds[5]));
				$flds[6] =~ m/(\d+)/ && $m->IndexNumber('length', $1);
			}
			elsif (substr($fld, 0, 1) eq 'R')
			{
				$m->IndexText('ref', substr($line, 5));
			}
			elsif ($fld eq 'FT')
			{
				# avoid indexing the sequences in the translation feature
				
				if ($text =~ m|/([^=]+)="(.+)"|)
				{
					$m->IndexText('ft', $2) if ($1 ne 'translation');
				}
				elsif ($line =~ m|/([^=]+)="(.+)|)
				{
					my $skip = $1 eq 'translation';
					
					$m->IndexText('ft', $2) unless $skip;
					
					while ($lookahead =~ /^FT\s+(.+)/ and substr($1, 0, 1) ne '/')
					{
						$m->IndexText('ft', $1) unless $skip;

						$doc .= $lookahead;
						
						$lookahead = <IN>;
					}
				}
			}			
			elsif ($fld ne 'SQ' and $fld ne 'XX' and $fld ne 'SV')
			{
				$m->IndexText(lc($fld), substr($line, 5));
			}
		}
	}
}

sub version
{
	my ($self, $raw_dir, $db) = @_;
	my $vers;
	
	if ($db eq 'embl_release')
	{
		open RELNOTES, "<$raw_dir/relnotes.txt";
		
		while (my $line = <RELNOTES>)
		{
			if ($line =~ /^\s+(Release\s+(\d+).+)/) {
				$vers = $1;
				last;
			}
		}

		close RELNOTES;
	}

	die "Unknown db: $db" unless defined $vers;

	chomp($vers);

	return $vers;
}

sub raw_files
{
	my ($self, $raw_dir) = @_;
	
	opendir DIR, $raw_dir;
	my @result = grep { -e "$raw_dir/$_" and $_ =~ /\.dat\.gz$/ } readdir DIR;
	closedir DIR;
	
	return map { "gunzip -c $raw_dir/$_ |" } sort @result;
}

1;
