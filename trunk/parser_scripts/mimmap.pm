# Perl module voor het parsen van pdb
#
# $Id: mimmap.pm 131 2006-08-10 12:02:09Z hekkel $
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

package mimmap::parser;

my $count = 0;

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "mimmap::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $m, $state);

	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	$state = 0;
	
	my $id = 0;

	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;
		chomp($line);
		
		if (substr($line, 0, 2) eq '//')
		{
			if (defined $doc)
			{
				$m->IndexValue('id', $id);
				
				$m->Store("ID      : $id\n$doc");
				$m->FlushDocument;
				
				++$id;
				
				$doc = undef;
			}
			
			$state = 0;
		}
		elsif ($state == 0)
		{
			if ($line =~ /^MIM#\s+:\s*(\S+)/o)
			{
				$m->IndexValue('id', $1);
			}
			elsif ($line =~ /^Date\s+:\s(\d+)\.(\d+)\.(\d+)/)
			{
				my ($year, $mon, $day) = ($1, $2, $3);
				$year += 1900;
				$year += 100 if $year < 1970;
				
				$m->IndexDate("date", sprintf("%4.4d-%2.2d-%2.2d", $year, $mon, $day));
			}
			elsif ($line =~ /^(.+?)\s*:(.*)/)
			{
				$m->IndexText(lc($1), $2);
			}
		}
	}

	if (defined $doc)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub raw_files
{
	my ($self, $raw_dir) = @_;
	
	$raw_dir =~ s|[^/]+/?$||;
	$raw_dir =~ s/raw/uncompressed/;
	
print "$raw_dir/omim/mimmap.txt\n";

	return "$raw_dir/omim/mimmap.txt";
}

1;
