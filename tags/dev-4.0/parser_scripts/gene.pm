# Perl module voor het parsen van pdb
#
# $Id: gene.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::gene;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		url			=> 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=gene',
		name		=> '(Entrez-) Gene',
		meta		=> [ 'title' ],
		section		=> 'gene',
		raw_files	=> qr/All_Data\.ags\.gz/,
		ext_mapping	=> {
			extension	=> qr/.+\.ags\.gz$/,
			command		=> '"./gene2xml -i $file -o stdout -b -c |"'
		},
		@_
	};
	return bless $self, "MRS::Script::gene";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my $m = $self->{mrs};
	
	my ($doc, $title, $last_id, $lookahead, $xml_header);
	my ($date_created, $date_updated, $date_discontinued);
	
	$lookahead = <IN>;
	while (defined $lookahead and not ($lookahead =~ /^\s*<Entrezgene-Set>/))
	{
		$xml_header .= $lookahead;	
		$lookahead = <IN>;
	}

	my $n = 0;
	my ($date_kind, %date);
	
	$date{create} = ();
	$date{update} = ();
	$date{discontinue} = ();
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;
		
		$line =~ s|^\s+||;
		
		if ($line =~ m|</Entrezgene>|)
		{
			foreach my $k ('create', 'update', 'discontinue')
			{
				next unless defined $date{$k}{year};
				my $date = sprintf("%4.4d-%2.2d-%2.2d", $date{$k}{year}, $date{$k}{month}, $date{$k}{day});
				$m->IndexDate("${k}d", $date);
			}
			
			$m->StoreMetaData('title', $title);
			$m->Store("$xml_header<Entrezgene-Set>\n$doc</Entrezgene-Set>\n");
			$m->FlushDocument;
			
			$doc = undef;
			$title = undef;

			$date_created = undef;
			$date_updated = undef;
			$date_discontinued = undef;
		}
		elsif ($line =~ m|^<Gene-track_geneid>(\d+)</Gene-track_geneid>|)
		{
			$m->IndexValue('id', $1);
		}
		elsif ($line =~ m/<Gene-track_(create|update|discontinue)-date>/)
		{
			$date_kind = $1;
		}
		elsif ($line =~ m'^<Date-std_(year|month|day)>(\d+)</Date-std_\1>')
		{
			$date{$date_kind}{$1} = $2;
		}
		elsif ($line =~ m|^<(.+?)>(.+)</\1>|)
		{
			my ($key, $text) = ($1, $2);
			
			$m->IndexText('text', $2);
			
			if ($key eq 'Prot-ref_name_E') {
				if (defined $title) {
					$title = "$title; $2";
				}
				else {
					$title = $2;
				}
			}
		}
	}
}

# formatting

my @links = (
	{
		match	=> qr|^(#=GF DR\s+PFAMA;\s)(\S+)(?=;)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfama&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PFAMB;\s)(\S+)(?=;)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfamb&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PDB;\s)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pdb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(#=GS .+AC )(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$2"}, $2)'
	},
);

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;

	$text =~ s/<\!DOCTYPE.+?>\n(<Entrezgene-Set>)?/<Entrezgene-Set>/;

	my $result;

	eval
	{
		open X, ">/tmp/input-$$.xml";
		print X $text;
		close X;

		our $mrs_bin_dir;
		
		my $settings = 'mrs.conf';
		unless (my $return = do $settings)
		{
			warn "couldn't parse $settings: $@" if $@;
			warn "couldn't do $settings: $!" unless defined $return;
			warn "couldn't run $settings" unless $return;
		}

		$mrs_bin_dir = "/usr/pkg/bin/" unless defined $mrs_bin_dir;
		my $mrs_script_dir = $this->script_dir();
	
		open X, "$mrs_bin_dir/Xalan /tmp/input-$$.xml $mrs_script_dir/gene_xslt.xml|";
		local($/) = undef;
		$result = <X>;
		close X;
	};
	
	if ($@)
	{
		$result = $q->pre($@);
	}
	unlink "/tmp/input-$$.xml";
	
	return $result;
}

1;

