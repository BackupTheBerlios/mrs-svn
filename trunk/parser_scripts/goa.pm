# Perl module voor het parsen van pdb
#
# $Id: goa.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::goa;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'GOA',
		url			=> 'http://www.ebi.ac.uk/GOA/',
		section		=> 'gene',
		meta		=> [ 'title' ],
		raw_files	=> qr/gene_association\.goa_uniprot\.gz$/,
		@_
	};
	return bless $self, "MRS::Script::goa";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my $m = $self->{mrs};
	
	my ($doc, $last_id, $title);
	
	$last_id = "";
	
	while (my $line = <IN>)
	{
		my @flds = split(m/\t/, $line);

		my $id = "$flds[0]:$flds[1]";
		
		if ($last_id ne $id and defined $doc)
		{
			$m->StoreMetaData('title', $title);
			
			$m->Store($doc);
			$m->FlushDocument;
			
			undef $doc;
			undef $title;
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
		
		$title = $flds[9] unless defined $title;

		$doc .= $line;
		$last_id = $id;
	}

	if (defined $doc)
	{
		$m->StoreMetaData('title', $title);

		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	my $result = "";
	
	foreach my $line (split(m/\n/, $text))
	{
		chomp($line);
		my @fields = split(m/\t/, $line);
	
		my @labels = (
			'DB: ',
			'DB_Object_ID: ',
			'DB_Object_Symbol: ',
			'Qualifier: ',
			'GOid: ',
			'DB:Reference: ',
			'Evidence: ',
			'With: ',
			'Aspect: ',
			'DB_Object_Name: ',
			'Synonym: ',
			'DB_Object_Type: ',
			'Taxon_ID: ',
			'Date: ',
			'Assigned_By: '
		);
	
		for (my $n = 0; $n < 15; ++$n)
		{
			my $value = $fields[$n];
	
			$value = "${url}?db=uniprot&id=$value'>$value</a>" if ($n == 2);
			$value = "${url}?db=uniprot&query=ac:$value'>$value</a>" if ($n == 1);
			$value =~ s|(GO:)(\d+)|${url}?db=go&id=$2'>$1$2</a>|g if ($n == 4);
			
			my $line = $labels[$n] . $value . "\n";
			
			$result .= $line;
		}
		
		$result .= "\n\n";
	}

	return $q->pre($result);
}

1;
