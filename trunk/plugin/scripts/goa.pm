# Perl module voor het parsen van pdb
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

package goa::parser;

my $count = 0;

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "goa::parser";
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
		
		if ($last_id ne $id and defined $doc)
		{
			$m->Store($doc);
			$m->FlushDocument;
			
			undef $doc;
		}

		$m->IndexText('db', $flds[0]);
# db_object_id
		$m->IndexText('acc', $flds[1]);
# db_object_symbol
		$m->IndexValue('id', $flds[2]);
		$m->IndexText('qualifier', $flds[3]);
		if ($flds[4] =~ /GO:(\d+)/)
		{
			$m->IndexWord('goid', $1);
		}
		$m->IndexText('db_reference', $flds[5]);
		$m->IndexText('evidence', $flds[6]);
		$m->IndexText('with', $flds[7]);
		$m->IndexText('aspect', $flds[8]);
		$m->IndexText('db_object_name', $flds[9]);
		$m->IndexText('synonym', $flds[10]);
		$m->IndexText('db_object_type', $flds[11]);
		$m->IndexTextAndNumbers('taxon_id', $flds[12]);
		if ($flds[13] =~ /(\d\d\d\d)(\d\d)(\d\d)/)
		{
			$m->IndexDate('date', "$1-$2-$3");
		}
		$m->IndexText('assigned_by', $flds[14]);

		$doc .= $line;
		$last_id = $id;
	}

	if (defined $doc)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub raw_files()
{
	my ($self, $raw_dir) = @_;
	return "gunzip -c $raw_dir/gene_association.goa_uniprot.gz |";
}

1;
