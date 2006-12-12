# MRS plugin for creating an enzyme.dat db
#
# $Id: enzyme.pm 26 2006-04-20 18:55:00Z maarten $
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

package MRS::Script::enzyme;

use strict;

our @ISA = "MRS::Script";

our %INDICES = (
	'id' => 'Identification',
	'de' =>	'Description',
	'an' => 'Alternate Name',
	'ca' => 'Catalytic Activity',
	'cf' => 'CoFactor',
	'cc' => 'Comments',
	'di' => 'Disease',
	'pr' => 'Prosite Reference',
	'dr' => 'Database Reference'
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Enzyme',
		url			=> 'http://ca.expasy.org/enzyme/',
		section		=> 'enzyme',
		meta		=> [ 'title' ],
		raw_files	=> qr/enzyme\.dat/,
		@_
	};
	return bless $self, "MRS::Script::enzyme";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $title, $m);
	
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

		if ($line eq '//')
		{
			$m->StoreMetaData('title', $title);
			
			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
			$title = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}(.+)/o)
		{
			my $fld = $1;
			my $text = $2;

			if ($fld eq 'ID' and $text =~ m/(\S+)/)
			{
				$id = $1;
				$m->IndexValue('id', $id);
			}
			elsif ($fld eq 'DE')
			{
				$m->IndexText('de', $text);
				$title .= ' ' if defined $title;
				$title .= $text;
			}
			else
			{
				my $txt = substr($line, 5);
				$m->IndexText(lc($fld), $txt) if $txt;
			}
		}
	}
}

# formatting

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	my $result;
	
	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'DR')
		{
			$line =~ s{
					(\S+)(,\s+)(\S+)(?=\s*;)
				}
				{
					"<a href='$url?db=uniprot\&query=ac:$1'>$1</a>" .
					"$2" .
					"<a href='$url?db=uniprot\&id=$3'>$3</a>"
				}xge;
		}
		
		$result .= "$line\n";
	}
	
	return $q->pre($result);
}

1;
