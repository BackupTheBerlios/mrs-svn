# Perl module voor het parsen van interpro
#
# $Id: interpro.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::interpro;

use XML::XSLT;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Interpro',
		url			=> 'http://www.ebi.ac.uk/interpro/',
		section		=> 'function',
		meta		=> [ 'title' ],
		raw_files	=> qr/interpro\.xml\.gz$/,
		@_
	};
	return bless $self, "MRS::Script::interpro";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	$| = 1;
	
	my $m = $self->{mrs};
	
	my ($doc, $last_id, $lookahead, $xml_header);
	
	$lookahead = <IN>;
	while (defined $lookahead and not ($lookahead =~ /^\s*<interpro\s/))
	{
		$xml_header .= $lookahead;	
		$lookahead = <IN>;
	}

	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;
		
		$line =~ s|^\s+||;
		
		if ($line =~ m|</interpro>|)
		{
			$m->Store($xml_header . $doc . "</interprodb>\n");
			$m->FlushDocument;
			
			$doc = undef;
		}
		elsif ($line =~ m|^<interpro|)
		{
			if ($line =~ m|id="(.+?)"|)
			{
				$m->IndexValue('id', $1);
			}

			if ($line =~ m|type="(.+?)"|)
			{
				$m->IndexText('type', $1);
			}

			if ($line =~ m|short_name="(.+?)"|)
			{
				$m->IndexText('name', $1);
			}
		}
		elsif ($line =~ m|^<db_xref|)
		{
			if ($line =~ m|dbkey="(.+?)"|)
			{
				$m->IndexText('xref', $1);
			}

			if ($line =~ m|name="(.+?)"|)
			{
				$m->IndexText('xref', $1);
			}
		}
		elsif ($line =~ m|^<taxon_data|)
		{
			if ($line =~ m|name="(.+?)"|)
			{
				$m->IndexText('taxon', $1);
			}
		}
		elsif ($line =~ m|^<classification|)
		{
			if ($line =~ m|id="(.+?)"|)
			{
				$m->IndexText('class', $1);
			}
		}
		elsif ($line =~ m|^<name>(.+)</name>|)
		{
			$m->StoreMetaData('title', $1);
			$m->IndexText('name', $1);
		}
		elsif ($line =~ m|^<(\w+?)>(.+)</\1>|)
		{
			$m->IndexText(lc($1), $2);
		}
		elsif ($line =~ m|^<rel_ref ipr_ref="(.+?)"|)
		{
			$m->IndexText("rel", $1);
		}
		elsif ($line =~ m|^<abstract|)
		{
			while (defined $lookahead and not ($lookahead =~ m|^\s*</abstract|))
			{
				$doc .= $lookahead;
				$m->IndexText('abstract', $lookahead);
				$lookahead = <IN>;
			}
		}
	}
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	my $script_dir = $this->script_dir;
	
	my $parser = XML::XSLT->new(Source => "$script_dir/interpro_xslt.xml");
	
	my $xml = $parser->serve($text, xml_declaration=>0, http_headers=>0);
	
	$xml =~ s|\[db_xref: (\S+?):(\S+?)\]|"<a href=\"$url?db=" . lc($1) . "&id=$2\">$2</a>"|eg;
	$xml =~ s|\[cite: PUB(\d+)\]|(<a href="http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=pubmed&cmd=search&term=$1">$1</a>)|g;
	
	$xml =~ s|#PUBMED#(\d+)|<a href="http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=pubmed&cmd=search&term=$1">$1</a>|g;
	$xml =~ s|#INTERPRO#(IPR\d+)|<a href="$url?db=interpro&amp;id=$1">$1</a>|g;
	$xml =~ s|#PDB#(.{4})|<a href="$url?db=pdb&amp;id=$1">$1</a>|g;
	
	$xml =~ s|&gt;|>|g;
	$xml =~ s|&lt;|<|g;
	$xml =~ s|&amp;|\&|g;

	return $xml;
}

1;
