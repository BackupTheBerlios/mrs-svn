# Perl module voor het parsen van pdb
#
# $Id: hssp.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::hssp;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		url			=> 'http://www.cmbi.kun.nl/gv/hssp/',
		name		=> 'HSSP',
		section		=> 'structure',
		meta		=> [ 'title' ],
		raw_files	=> qr/\.hssp$/,
		@_
	};
	return bless $self, "MRS::Script::hssp";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $m, $state);

	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	
	$state = 0;

	# like dssp, assume there's only one record per parse call
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;
		chomp($line);
			
		if ($state == 0 and $line =~ /^PDBID\s+(\S+)/o)
		{
			my $id = $1;
			
			$m->IndexValue('id', $id);
			
			$state = 1;
		}
		elsif ($state == 1)
		{
			if ($line =~ /^## ALIGNMENTS/)
			{
				$state = 2;
			}
			elsif ($line =~ /^(\S+?)\s+(.*)/)
			{
				my $fld = $1;
				$line = $2;
				
				$line =~ s/(\w)\.(?=\w)/$1. /og
					if ($fld eq 'AUTHOR');

				$m->StoreMetaData('title', lc($line))
					if ($fld eq 'HEADER');

				$m->IndexText('text', $line);
			}
		}
	}

	$m->Store($doc);
	$m->FlushDocument;
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(\S+)|<a href='$&'>$&</a>|g;

	$text =~ s{
		^(\s+\d+\s:\s)(\S+)(\s+)(\d\w\w\w)?(?=\s+\d)
	}
	{
		if (defined $4 and length($4))
		{
			"$1" . $q->a({-href=>"$url?db=uniprot&id=$2"}, $2) . $3 . 
				$q->a({-href=>"$url?db=pdb&id=$4"}, $4);
		}
		else {
			"$1" . $q->a({-href=>"$url?db=uniprot&id=$2"}, $2) . $3;
		}
	}mxge;
	
	return $q->pre($text);
}

1;
