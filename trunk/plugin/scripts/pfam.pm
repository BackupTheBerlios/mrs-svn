# MRS plugin for creating an pfam db
#
# $Id: pfam.pm,v 1.4 2005/08/22 12:38:08 maarten Exp $
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

package pfam::parser;

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
	return bless $self, "pfam::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $state, $m);
	
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

			$doc = undef;
		}
		elsif (substr($line, 0, 5) eq '#=GF ')
		{
			my $field = substr($line, 5, 2);
			my $value = substr($line, 10);

			if ($field eq 'ID')
			{
				$m->IndexValue('id', $value);
			}
			elsif ($field eq 'AC' and $value =~ m/(P[BF]\d+)/)
			{
				$m->IndexValue('ac', $1);
			}
			elsif (substr($field, 0, 1) eq 'R')
			{
				$m->IndexText('ref', substr($line, 5));
			}
			else
			{
				$m->IndexText(lc($field), substr($line, 5));
			}
		}
		elsif (substr($line, 0, 5) eq '#=GS ')
		{
			$m->IndexText('gs', substr($line, 5));
		}
	}
}

sub raw_files
{
	my ($self, $raw_dir, $db) = @_;

	$raw_dir =~ s|/[^/]+/?$|/|;
	
	my $result;

	$result = "gunzip -c $raw_dir/pfam/Pfam-A.full.gz|" if ($db eq 'pfama');
	$result = "gunzip -c $raw_dir/pfam/Pfam-A.seed.gz|" if ($db eq 'pfamseed');
	$result = "gunzip -c $raw_dir/pfam/Pfam-B.gz|" if ($db eq 'pfamb');
	
	die "unknown pfam db: $db\n" unless defined $result;
	
	return $result;
}

1;
