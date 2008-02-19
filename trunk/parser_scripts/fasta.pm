# MRS plugin for creating a db
#
# $Id: uniseq.pm 169 2006-11-10 08:02:05Z hekkel $
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

package MRS::Script::fasta;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		url			=> 'http://www.undefined/',
		section		=> 'protein',
		meta		=> [ 'title' ],
		raw_files	=> qr/.+\.fa(sta)?$/,
		@_
	};
	
	$self->{name} = $self->{$db} if defined $self->{db};
	
	return bless $self, "MRS::Script::fasta";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $m, $seq);
	
	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		if ($line =~ m/>(\S+)\s+(.+)/)
		{
			if (defined $doc)
			{
				$m->Store($doc);
				$m->AddSequence($seq);
				$m->FlushDocument;
			}

			$doc = $line;
			$seq = undef;

			$m->IndexValue('id', $1);
			$m->StoreMetaData('title', substr($2, 0, 255));
			$m->IndexText('title', $2);
		}
		elsif ($line =~ m/>(\S+)/)
		{
			if (defined $doc)
			{
				$m->Store($doc);
				$m->AddSequence($seq);
				$m->FlushDocument;
			}

			$doc = $line;
			$seq = undef;

			$m->IndexValue('id', $1);
		}
		else
		{
			$doc .= $line;
			chomp($line);
			
			$seq .= $line;
		}
	}
	
	if (defined $doc)
	{
		$m->Store($doc);
		$m->AddSequence($seq);
		$m->FlushDocument;
	}
}

1;
