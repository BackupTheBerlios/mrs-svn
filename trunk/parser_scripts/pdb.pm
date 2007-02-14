# Perl module voor het parsen van pdb
#
# $Id: pdb.pm 79 2006-05-30 08:36:36Z maarten $
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

package MRS::Script::pdb;

our @ISA = "MRS::Script";

my %aa_map = (
	'ALA'	=>	'A',
	'ARG'	=>	'R',
	'ASN'	=>	'N',
	'ASP'	=>	'D',
	'CYS'	=>	'C',
	'GLN'	=>	'Q',
	'GLU'	=>	'E',
	'GLY'	=>	'G',
	'HIS'	=>	'H',
	'ILE'	=>	'I',
	'LEU'	=>	'L',
	'LYS'	=>	'K',
	'MET'	=>	'M',
	'PHE'	=>	'F',
	'PRO'	=>	'P',
	'SER'	=>	'S',
	'THR'	=>	'T',
	'TRP'	=>	'W',
	'TYR'	=>	'Y',
	'VAL'	=>	'V',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'PDB',
		url			=> 'http://www/rcsb.org/pdb/',
		section		=> 'structure',
		meta		=> [ 'title' ],
		compression	=> 'bzip',
		@_
	};
	return bless $self, "MRS::Script::pdb";
}

sub GetTitle
{
	my ($header, $compound, $title) = @_;
	
	my $desc;
		
	if ($title and $title ne '')
	{
		$desc = "$title ($header)";
	}
	else
	{
		$desc = $header;
	}

	if ($compound and $compound ne '')
	{
		$desc .= "; $compound";
	}	
	
	return $desc;
}

sub parse
{
	my ($self, $verbose);
	local *IN;
	
	($self, *IN, $verbose) = @_;
	
	my ($doc, $state, $m, %seq, $sequence, $seq_chain_id, $header, $compound, $title);
	
	$state = 0;
	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		if ($line =~ /^HEADER\s+(.+?)\d{2}-[A-Z]{3}-\d{2}\s+(\S\S\S\S)/o)
		{
			if (defined $doc)
			{
				$m->StoreMetaData('title', &GetTitle($header, $title, $compound));
				$m->Store($doc);

				foreach my $ch (keys %seq)
				{
					$m->AddSequence($seq{$ch});
					delete $seq{$ch};
				}
		
				$m->AddSequence($sequence) if defined $sequence;
				
				$sequence = undef;
				$seq_chain_id = undef;

				$m->FlushDocument;
			}
			
			$doc = $line;
			$header = $1;
			$title = undef;
			$compound = undef;
			
			$m->IndexText('text', $1);
			$m->IndexValue('id', $2);

			$state = 1;
		}
		elsif ($state == 1)
		{
			$doc .= $line;
			chomp($line);
			
			if ($line =~ /^(\S+)\s+(\d+\s+)?(.+)/o)
			{
				my ($fld, $text) = ($1, $3);

#				if ($fld eq 'ATOM')
#				{
#					$state = 2;
#				}
#				els
				if ($fld eq 'TITLE')
				{
					$title .= lc $text;
					$m->IndexText('title', $text);
				}
				elsif ($fld eq 'COMPND')
				{
					if ($text =~ m/MOLECULE: (.+)/) {
						$compound .= lc $1 . ' ';
					}
					elsif ($text =~ m/EC: (.+)/) {
						$compound .= 'EC: ' . lc $1 . ' ';
					}
					$m->IndexText('compnd', $text);
				}
				elsif ($fld eq 'AUTHOR')
				{
					# split out the author name, otherwise users won't be able to find them
					
					$text =~ s/(\w)\.(?=\w)/$1. /og;
					$m->IndexText('ref', $text);
				}
				elsif ($fld eq 'JRNL')
				{
					$m->IndexText('ref', $text);
				}
				elsif ($fld eq 'REMARK')
				{
					if ($text =~ /\s*(\d+\s+)?AUTH\s+(.+)/o)
					{
						$text = $2;
						$text =~ s/(\w)\.(?=\w)/$1. /og;
					}
					
					$m->IndexText('remark', $text);
				}
				elsif ($fld eq 'SEQRES')
				{
					my $chain_id = substr($line, 11, 1);
					my $s = substr($line, 19, 52);
					
					if (defined $seq_chain_id and
						defined $chain_id and
						$chain_id ne $seq_chain_id and
						defined $sequence)
					{
						if (length($sequence) > 2) {
							$seq{$seq_chain_id} = $sequence;
						}
						$sequence = undef;
					}

					$seq_chain_id = $chain_id;
					
					foreach my $aa (split(m/\s+/, $s))
					{
						if (length($aa) == 3 and defined $aa_map{$aa})
						{
							$sequence = "" unless defined $sequence;
							$sequence .= $aa_map{$aa};
						}
					}
				}
				elsif ($line =~ /^\S+\s+(.*)/o)
				{
					$m->IndexText('text', $1);
				}
			}
		}
		else
		{
			$doc .= $line;
		}
	}

	if (defined $doc)
	{
		$m->StoreMetaData('title', &GetTitle($header, $title, $compound));
		$m->Store($doc);
		
		foreach my $ch (sort keys %seq)
		{
			$m->AddSequence($seq{$ch});
			delete $seq{$ch};
		}

		$m->AddSequence($sequence) if defined $sequence;
		
		$m->FlushDocument;
	}
}

sub raw_files()
{
	my ($self) = @_;

	my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";
	
	my @result;
	
	opendir DIR, $raw_dir;
	while (my $d = readdir DIR)
	{
		next unless $d =~ m/^[0-9a-z]{2}$/;
		
		opendir D2, "$raw_dir/$d";
		my @files = grep { -e "$raw_dir/$d/$_" and $_ =~ /\.ent\.Z$/ } readdir D2;
		push @result, map { "$raw_dir/$d/$_" } @files if scalar @files;
		closedir D2;
	}
	closedir DIR;
	
	return @result;
}

# formatting

my $jmol_script=<<END;
<script src="jmol/Jmol.js">
</script>
<script language="JavaScript">
	jmolInitialize("jmol");
	jmolCheckBrowser("popup", "/browsercheck", "onClick");
	
	jmolApplet(250, "load %s?db=pdb&amp;id=%s&amp;exp=1&amp;save_to=text%%2Fplain;" +
		" cpk off; wireframe off; cartoon; color cartoon structure; spin");
</script>
END

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;

	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	my $script = sprintf($jmol_script, 'plain.do', $id);
	
	$text =~ s|(COMPND   \d? )EC: ((\d+\.){3}\d+)|$1<a href="$url?id=enzyme%3A$2">EC: $2</a>|gmo;
	
	return $script . $q->pre($text);
}

1;
