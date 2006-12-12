# Perl module voor het parsen van hugo
#
# $Id: hugo.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::hugo;

@ISA = "MRS::Script";

my $count = 0;

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Hugo nomenclature',
		url			=> 'http://www.gene.ucl.ac.uk/nomenclature/',
		section		=> 'gene',
		meta		=> [ 'title' ],
		raw_files	=> qr/nomeids\.txt/,
		@_
	};
	return bless $self, "MRS::Script::hugo";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my $m = $self->{mrs};
	
	my ($doc, $last_id);
	
	$last_id = "";
	
	while (my $line = <IN>)
	{
		my @flds = split(m/\t/, $line);

		my $id = "$flds[0]:$flds[1]";
		
#HGNC
		$m->IndexValue('id', $flds[0]);
#Symbol
		$m->IndexTextAndNumbers('symbol', $flds[1]);
#Name
		$m->IndexTextAndNumbers('name', $flds[2]);
#Map
		$m->IndexTextAndNumbers('map', $flds[3]);
#MIM
		$m->IndexTextAndNumbers('mim', $flds[4]);
#PMID1
		$m->IndexTextAndNumbers('pmid1', $flds[5]);
#PMID2
		$m->IndexTextAndNumbers('pmid2', $flds[6]);
#Ref Seq
		$m->IndexTextAndNumbers('refseq', $flds[7]);
#Aliases
		$m->IndexTextAndNumbers('aliases', $flds[8]);
#Withdrawn Symbols
		$m->IndexTextAndNumbers('withdrawn', $flds[9]);
#Locus Link
		$m->IndexTextAndNumbers('locuslink', $flds[10]);
#GDB ID
		$m->IndexTextAndNumbers('gdb', $flds[11]);
#SWISSPROT
		$m->IndexTextAndNumbers('swissprot', $flds[12]);

		$m->StoreMetaData('title', $flds[2]);

		$m->Store($line);
		$m->FlushDocument;
	}
}

1;
