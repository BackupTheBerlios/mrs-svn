# MRS plugin for creating an EMBL db
#
# $Id: locuslink.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::locuslink;

our @ISA = "MRS::Script";

use strict;

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'LocusLink',
		url			=> 'http://www.ncbi.nlm.nih.gov/projects/LocusLink/',
		section		=> 'other',
		meta		=> [ 'title' ],
		raw_files	=> qr/LL_tmpl\.gz/,
		@_
	};
	return bless $self, "MRS::Script::locuslink";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $m, $title);
	
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		if ($line =~ /^>>\d+/)
		{
			if (defined $doc)
			{
				$m->StoreMetaData('title', $title);
				$m->Store($doc);
				$m->FlushDocument;
			}
			
			$doc = $line;
		}
		else
		{
			$doc .= $line;
			chomp($line);

			if ($line =~ /^LOCUSID:\s+(\d+)/)
			{
				$m->IndexValue('id', $1);
			}
			elsif ($line =~ /(.+?):(.*)/)
			{
				my $fld = lc $1;
				my $value = $2;

				if ($fld eq 'preferred_gene_name' or $fld eq 'preferred_product') {
					$title = $value;
				}
				elsif ($fld eq 'product' and not defined $title) {
					$title = $value;
				}
				
				my %fn = (
					'organism' => 1,
					'product' => 1,
					'summary' => 1,
					'omim' => 1,
					'chr' => 1
				);
				
				$fld = 'text' unless (defined $fn{$fld});
				
				$m->IndexText($fld, $value);
			}
		}
	}
	
	if (defined $doc)
	{
		$m->StoreMetaData('title', $title);
		$m->Store($doc);
		$m->FlushDocument;
	}
}

1;
