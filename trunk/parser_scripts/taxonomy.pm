#!/usr/bin/perl -w
#
# $Id: taxonomy.pm 79 2006-05-30 08:36:36Z maarten $
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

package MRS::Script::taxonomy;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Taxonomy',
		section		=> 'other',
		url			=> 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=Taxonomy',
		meta		=> [ 'title' ],
		raw_files	=> qr/taxonomy\.dat/,
		@_
	};
	return bless $self, "MRS::Script::taxonomy";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $m, $sn, $cn);
	
	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		$doc .= $line;

		chomp($line);
		
		if ($line eq '//')
		{
			my $n = $sn;
			$n .= " $cn" if defined $cn;
			
			$m->StoreMetaData('title', $n);

			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
			$sn = undef;
			$cn = undef;
		}
		elsif ($line =~ /^(\S+(\s\S+)*)\s+:\s+(.+)/o)
		{
			my $fld = $1;
			my $value = $3;

			if ($fld eq 'ID')
			{
				$m->IndexValue('id', $value);
			}
			elsif ($fld eq 'PARENT ID')
			{
				$m->IndexTextAndNumbers('parent', $value);
			}
			else
			{
				$sn = $value if $fld eq 'SCIENTIFIC NAME';
				$cn = $value if $fld eq 'COMMON NAME';
				
				$m->IndexText('text', $value);
				
				$fld =~ s/\s/_/g;
				$m->IndexText(lc $fld, $value);
			}
		}
	}
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(\S+)|<a href='$&'>$&</a>|g;

	$text =~ s|^(PARENT ID\s+:\s+)(\d+)|$1<a href=$url?db=taxonomy&id=$2>$2</a>|mo;
	
	return $q->pre($text);
}

1;
