# MRS plugin for creating an prosite.dat db
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

package prosite::parser;

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
	return bless $self, "prosite::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	
	$lookahead = <IN>
		while (substr($lookahead, 0, 2) eq 'CC');
	$lookahead = <IN>
		if (substr($lookahead, 0, 2) eq '//');
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		$doc .= $line;

		chomp($line);

		if (substr($line, 0, 2) eq '//')
		{
			die "No ID specified\n" unless defined $id;

			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}/o)
		{
			my $fld = $1;
			my $value = substr($line, 5);

			if ($fld eq 'ID' and $value =~ /^([A-Z0-9_]+)/o)
			{
				die "Double ID: $id <=> $1\n" if defined $id;
				$id = $1;
				die "ID too short" unless length($id);
				$m->IndexValue('id', $id);
			}
			else
			{
				$m->IndexText(lc($fld), substr($line, 5));
			}
		}
	}
}

sub raw_files
{
	my ($self, $raw_dir) = @_;
	
	return "<$raw_dir/prosite.dat";
}

1;
