# MRS plugin for creating an EMBL db
#
# $Id: dbest.pm 18 2006-03-01 15:31:09Z hekkel $
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

package dbest::parser;

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
	return bless $self, "dbest::parser";
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

		if ($line eq '||')
		{
			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
		}
		else
		{
			if ($line =~ m/^dbEST Id:\s+(\d+)/)
			{
				$m->IndexValue('id', $1);
			}
			elsif ($line =~ m/^EST name:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('source_est', $1);
			}
			elsif ($line =~ m/^Clone Id:\s+(.+)/)
			{
				$m->IndexText('clone', $1);
			}
			elsif ($line =~ m/^GenBank gi:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('genbank_gi', $1);
			}
			elsif ($line =~ m/^GenBank Acc:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('genbank_acc', $1);
			}
			elsif ($line =~ m/^Source:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('source', $1);
			}
			elsif ($line =~ m/^Organism:\s+(.+)/)
			{
				$m->IndexText('organism', $1);
			}
			elsif ($line =~ m/^Lib Name:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('lib_name', $1);
			}
			elsif ($line =~ m/^dbEST Lib id:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('lib_id', $1);
			}
			elsif ($line =~ m/^DNA Type:\s+(.+)/)
			{
				$m->IndexText('dna_type', $1);
			}
			elsif ($line =~ m/^Map:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('chr', $1);
			}
			elsif ($line =~ m/^GDB Id:\s+(.+)/)
			{
				$m->IndexTextAndNumbers('gdb', $1);
			}
			elsif ($line =~ m/^SEQUENCE/)
			{
				while (defined $lookahead and length($lookahead) > 0 and substr($lookahead, 0, 1) eq ' ')
				{
					$lookahead = <IN>;
				}
			}
			elsif (length($line) > 16)
			{
				$m->IndexText('text', substr($line, 16, length($line) - 16));
			}
		}
	}
}

sub raw_files
{
	my ($self, $raw_dir) = @_;
	
	opendir DIR, $raw_dir;
	my @result = grep { -e "$raw_dir/$_" and $_ =~ /\.gz$/ } readdir DIR;
	closedir DIR;
	
	return map { "gunzip -c $raw_dir/$_ |" } @result;
}

1;
