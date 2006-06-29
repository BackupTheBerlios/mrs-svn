# $Id: Format_embl.pm,v 1.18 2005/10/27 10:59:11 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_embl;

use Data::Dumper;
use POSIX qw(strftime);
use Time::Local;

our @ISA = "Format";

my @links_db_xref = (
	{
		match	=> qr|^(embl:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=embl_release|embl_updates&query=ac:$2"}, $2)'
	},
	{
		match	=> qr[^(swiss-prot|sptrembl|uniprot/swiss-prot|uniprot/trembl|UniProtKB/Swiss-Prot):(\S+)]i,
		result	=> '$1.":".$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(taxon:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=taxonomy&id=$2"}, $2)'
	},
	{
		match	=> qr|^(GO; GO:)(\d+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=go&id=$2"}, $2)'
	},
	{
		match	=> qr|^(GOA:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=goa&query=acc:$2"}, $2)'
	},
	{
		match	=> qr|^(InterPro:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=interpro&id=$2"}, $2)'
	},
	{
		match	=> qr|^(Pfam:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfamseed|pfama|pfamb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(PDB:)(\d\w\w\w)|i,
		result	=> '"PDB:".$q->a({-href=>"$url?db=pdb&id=$2"}, "$2")'
	},
);

my %links = (
	'EMBL'	=>	{
		match	=> qr|^(\S+?)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=embl_release|embl_updates&query=ac:$1"}, $1)'
	},

	'UNIPROT'	=> {
		match	=> qr[^(\S+)(?=;)]i,
		result	=> '$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$1"}, $1)'
	},

	'PDB'		=> {
		match	=> qr|^(\d\w\w\w)|i,
		result	=> '$q->a({-href=>"$url?db=pdb&id=$1"}, $1)'
	},

	'HSSP'		=> {
		match	=> qr|^(\S+;\s)(\d\w\w\w)|i,
		result	=> '$1.$q->a({-href=>"$url?db=hssp&id=$2"}, $2)'
	},

	'GO'		=> {
		match	=> qr|^(GO:)(\d+)|i,
		result	=> '$q->a({-href=>"$url?db=go&id=$2"}, $2)'
	},

	'Pfam'		=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=pfamseed|pfama|pfamb&query=ac:$1"}, $1)'
	},

	'InterPro'	=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=interpro&id=$1"}, $1)'
	},

	'MIM'		=> {
		match	=> qr|^(\d+)|i,
		result	=> '$q->a({-href=>"$url?db=omim&id=$1"}, $1)'
	},

	'GDB'		=> {
		match	=> qr|^(\S+?)\.|,
		result	=> '$q->a({-href=>"http://www.gdb.org/gdb-bin/genera/accno?accessionNum=$1"}, $1)'
	}
);

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'embl',
		@_
	};
	my $result = bless $self, "Format_embl";
	return $result;
}

sub print_ref
{
	my ($q, $ref) = @_;
	my @rr;

	foreach my $rx (@{$ref->{rx}})
	{
		my ($name, $ident) = split(m/[=:;]\s*/, $rx);
		
		$ident =~ s/\.$//;
		
		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?cmd=Retrieve&db=PubMed&list_uids=$ident&dopt=Abstract"}, $ident)
			if (lc $name eq 'medline');

		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=pubmed&cmd=search&term=$ident"}, $ident)
			if (lc $name eq 'pubmed');
		
		$ident = $q->a({-href=>"http://dx.doi.org/$ident"}, $ident)
			if ($name eq 'DOI');
		
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, $name), $q->td($ident));
	}
	
	if ($ref->{rp})
	{
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Position'), $q->td(lc $ref->{rp}));
	}
	
	my @rc = split(m/;\s*/, $ref->{rc});
	foreach my $rc (@rc)
	{
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Comment'), $q->td(lc $rc));
	}

	push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Group'), $q->td($ref->{rg}))
		if defined $ref->{rg};
	
	my @l;
	push @l, $ref->{ra};
	push @l, $q->strong($ref->{rt});
	push @l, $ref->{rl};
	push @l, '<br/>';
	push @l, $q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @rr) if scalar @rr;
	
	return join(" ", @l);
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	my @rows;
	my (%ref, @ref_rows);
	
	my @lines = split(m/\n/, $text);
	my (@os, @oc, $og, @ox);
	my $kw_ix;
	
	my $lookahead = shift @lines;
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = shift @lines;
		
		# first block, the entry information
		if ($line =~ /^ID\s+(\S+)/)
		{
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Entry information'));
			push @rows, $q->Tr($q->td({-width=>'20%'}, 'Entry name'), $q->td($q->strong($1)));
		}
		elsif ($line =~ /^AC\s+(.+)/)
		{
			my @acc;
			
			for (;;)
			{
				push @acc, split(m/;\s*/, $1);
				
				last unless $lookahead =~ /^AC(.+)/;
				$lookahead = shift @lines;
			}

			push @rows, $q->Tr($q->td('Primary accession'), $q->td($q->strong(shift @acc)));
			push @rows, $q->Tr($q->td('Secondary accessions'), $q->td(join(' ', @acc)))
				if (scalar @acc);
		}
		elsif ($line =~ /^SV\s+(\S+)/)
		{
			push @rows, $q->Tr($q->td('Sequence Version'), $q->td($1));
		}
		elsif ($line =~ /^DT\s+(\d{2})-([A-Z]{3})-(\d{4}) \(Rel. (\d+), (.+?)(, Version (\d+))?\)/)
		{
			my %months = ( 'JAN' => 0, 'FEB' => 1, 'MAR' => 2, 'APR' => 3, 'MAY' => 4, 'JUN' => 5, 
				 'JUL' => 6, 'AUG' => 7, 'SEP' => 8, 'OCT' => 9, 'NOV' => 10, 'DEC' => 11);
			
			my $release = $4;
			my $desc = $5;
			my $date = strftime("%B %Y", 0, 0, 0, $1, $months{$2}, $3 - 1900);
			my $version = $7;
			
			my $s = "Release $release, $date";
			$s .= " (Version $version)" if defined $version;
			
			push @rows, $q->Tr($q->td($desc), $q->td($s));
		}

		# second block, name and origin
		
		elsif ($line =~ /^DE\s+(.+)/)
		{
			my $de = $1;
			
			while ($lookahead =~ /DE\s+(.+)/)
			{
				$de .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Description'));
			push @rows, $q->Tr($q->td('Description'), $q->td($de));
			
			$kw_ix = scalar @rows;
		}

		elsif ($line =~ /^KW\s+(.+?)$/ and $1 ne '.')
		{
			my @kw = split(m/;\s*/, $1);
			
			while ($lookahead =~ /^KW\s+(.+?)$/)
			{
				push @kw, split(m/;\s*/, $1);
				$lookahead = shift @lines;
			}

			my @kwr;
			push @kwr, $q->Tr($q->td('Keywords'), $q->td(join('; ', @kw)));
			splice(@rows, $kw_ix, 0, @kwr);
		}

		elsif ($line =~ /^OS\s+(.+?)(\.|,( and)?)?$/)
		{
			push @os, $1;
		}
		elsif ($line =~ /^OC\s+(.+?)(\.|;)?$/)
		{
			push @oc, split(m/;\s*/, $1);
		}
		elsif ($line =~ /^OG\s+(.+)/)
		{
			$og .= ';' if $og;
			$og .= $1;
		}
		
		# reference part
		
		elsif ($line =~ /^RN\s+\[(\d+)\]/)
		{
			my $nr = $1;
			
			# first flush the O part
			if (@os)
			{
#				die "mismatch OS " . scalar @os . " OX " . scalar @ox  if scalar @os != scalar @ox;
				
				my @osr;
				
				for (my $i = 0; $i < scalar @os; ++$i)
				{
					my $ox = $ox[$i];
					my $os = $os[$i];
					
					if ($ox)
					{
						my $taxon_url = $q->a({-href=>"$url?db=taxonomy&id=$ox"}, $ox);
						$os = "$os (Taxon ID: $taxon_url)";
					}
					push @osr, $q->td($os);
				}
				
				push @rows, $q->Tr($q->td({-rowspan=>(scalar @osr)}, 'From'), shift @osr);
				push @rows, $q->Tr(\@osr) if scalar @osr;
				
				push @rows, $q->Tr($q->td('Encoded on'), $q->td($og)) if $og;
				
				push @rows, $q->Tr($q->td('Taxonomy'), $q->td(
					join("; ", map { $q->a({-href=>"$url?db=taxonomy&query=$_"}, $_) } @oc)));
				
				undef @os;
				undef $og;
				undef @ox;
				undef @oc;

				push @rows, $q->Tr($q->th({-colspan=>2}, 'References'));
			}
			
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			
			undef %ref;
			$ref{nr} = $nr;
		}
		
		elsif ($line =~ /^RP\s+(.+)/)
		{
			$ref{rp} .= "$1 ";
		}
		elsif ($line =~ /^RX\s+(.+)/)
		{
			my @rx = ();
			$ref{rx} = \@rx unless $ref{rx};
			push @{$ref{rx}}, $1;
		}
		elsif ($line =~ /^RC\s+(.+)/)
		{
			$ref{rc} .= "$1 ";
		}
		elsif ($line =~ /^RG\s+(.+)/)
		{
			$ref{rg} .= "$1 ";
		}
		elsif ($line =~ /^RA\s+(.+)/)
		{
			$ref{ra} .= "$1 ";
		}
		elsif ($line =~ /^RT\s+(.+?);?$/)
		{
			$ref{rt} .= "$1 ";
		}
		elsif ($line =~ /^RL\s+(.+?)\.?$/)
		{
			$ref{rl} .= "$1 ";
		}

		elsif ($line =~ /^XX/)
		{
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			undef %ref;
		}
		
		# cross reference
		
		elsif ($line =~ /^DR\s+(.+)/)
		{
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			undef %ref;
			
			my @dr;
			my $last_db;

			push @rows, $q->Tr($q->th({-colspan=>2}, 'Cross-references'));
			
			for (;;)
			{
				my ($db, $ids) = split(m/;\s*/, substr($line, 5), 2);

				if ($db ne $last_db and scalar @dr)
				{
					
					push @rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
					push @rows, $q->Tr(\@dr) if scalar @dr;
					
					undef @dr;
				}
				
				if (my $l = $links{$db})
				{
					$ids =~ s/$l->{match}/$l->{result}/eegm;
				}
				
				push @dr, $q->td($ids);
				$last_db = $db;
				
				last if (substr($lookahead, 0, 2) ne 'DR');
				$line = $lookahead;
				$lookahead = shift @lines;
			}

			push @rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
			push @rows, $q->Tr(\@dr) if scalar @dr;
		}
		
		# comments
		
		elsif ($line =~ /^CC\s*(.+)/)
		{
			my $cc = $1;
			
			while ($lookahead =~ /CC\s+(.+)/)
			{
				$cc .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Comment'));
			push @rows, $q->Tr($q->td({-colspan=>2}, $cc));
		}
	
		# AH/AS ... not implemented yet
		
		# CO
		
		elsif ($line =~ /^CO\s*(.+)/)
		{
			my $co = $1;
			
			while ($lookahead =~ /CO\s+(.+)/)
			{
				$co .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Constructed'));
			push @rows, $q->Tr($q->td('Contigs'), $q->td($co));
		}
		
		# Feature table
		
		elsif ($line =~ /^FT/)
		{
			my @ft;
			
			push @ft, $q->Tr(
				$q->th({-width=>'10%'}, 'Key'),
				$q->th({-width=>'10%'}, 'Location'),
				$q->th({-width=>'10%'}, 'Qualifier'),
				$q->th({-width=>'70%'}, 'Value'));
			
			for (;;)
			{
				my $key = substr($line, 5, 8);		$key =~ s/\s+$//;
				my $location = substr($line, 21);	$location =~ s/\s+$//;
				
				my (@fd, $f);
				
				while ($lookahead =~ m|^FT\s{10,}([^/\s].+)|)
				{
					$location .= " $1";
					$lookahead = shift @lines;
				}
				
				while ($lookahead =~ /^FT\s{10,}(.+)/)
				{
					my $nf = $1;
					
					if ($f and length($f) and $nf =~ m|/(\S+?)=| and $f =~ m|/(\S+?)=(.+)|)
					{
						# unfinished feature line... see if we can fix it

						push @fd, $q->td($1) . $q->td($2);
						$f =~ s|/(\S+?)=(.+)||;
					}
					
					$f .= " $nf";
					
					if ($f =~ m|/(\S+?)="(.+)"|)
					{
						my $fq = $1;	$fq =~ s/\s+$//;
						my $fv = $2;	$fv =~ s/\s+$//;
						
						if ($fq eq 'translation')
						{
							$fv =~ s/\s//g;
							$fv =~ s/(.{30})/$1\n/g;
							$fv = $q->div({-id=>'translation-box'}, $fv);
						}
						elsif ($fq eq 'db_xref')
						{
							foreach my $l (@links_db_xref)
							{
								$fv =~ s/$l->{match}/$l->{result}/eem;
							}							
						}
						elsif ($fq eq 'EC_number')
						{
							$fv = $q->a({-href=>"$url?db=enzyme&id=$fv"}, $fv); 
						}
						
						push @fd, $q->td($fq) . $q->td($fv);
						$f =~ s|/(\S+?)="(.+)"||;
					}

					if ($f =~ m|/(\S+?)=(\d+)|)
					{
						push @fd, $q->td($1) . $q->td($2);
						$f =~ s|/(\S+?)=(\d+)||;
					}
					
					$lookahead = shift @lines;
				}
				
				$location =~ s/,/, /g;
				
				if (scalar @fd > 1)
				{
					push @ft, $q->Tr(
						$q->td({-rowspan=>(scalar @fd)}, $key),
						$q->td({-rowspan=>(scalar @fd)}, $location),
						shift @fd);
				}
				else
				{
					push @ft, $q->Tr(
						$q->td($key),
						$q->td($location),
						shift @fd);
				}

				push @ft, $q->Tr(\@fd) if scalar @fd;
				
				last unless $lookahead =~ /^FT/;
				
				$line = $lookahead;
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Features'));
			push @rows, $q->Tr($q->td({-colspan=>2}, $q->div({-class=>'sub_entry'},
					$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @ft))));
		}
		
		elsif ($line =~ /^SQ\s*Sequence\s*(\d+)\s*BP;\s*(.+)/)
		{
			my $bp = $1;

			my $c = $2;
			my ($ac, $cc, $tc, $gc, $oc);
			
			$ac = $1 if $c =~ /(\d+)\s*A;/;
			$cc = $1 if $c =~ /(\d+)\s*C;/;
			$tc = $1 if $c =~ /(\d+)\s*T;/;
			$gc = $1 if $c =~ /(\d+)\s*G;/;
			$oc = $1 if $c =~ /(\d+)\s*Other;/;
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Sequence information'));
			
			my $cnt = "Length: <strong>$bp</strong> BP";
			$cnt .= ", A Count: <strong>$ac</strong>" if $ac;
			$cnt .= ", C Count: <strong>$cc</strong>" if $cc;
			$cnt .= ", T Count: <strong>$tc</strong>" if $tc;
			$cnt .= ", G Count: <strong>$gc</strong>" if $gc;
			$cnt .= ", Other Count: <strong>$oc</strong>" if $oc;
			
			push @rows, $q->Tr($q->td({-colspan=>2}, $cnt));

			my $seq;
			while (not ($lookahead =~ m|//|))
			{
				$lookahead =~ s/^\s+//;
				
				$seq .= "$lookahead\n";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->td({-colspan=>2}, $q->pre($seq)));
		}
		
	}

	return $q->div({-class=>'entry'},
		$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @rows));

}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	my $state = 0;

	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'DE')
		{
			$state = 1;
			$desc .= substr($line, 4);
		}
		elsif ($state == 1)
		{
			last;
		}
	}
	
	return $desc;
}

sub to_fasta
{
	my ($this, $q, $text) = @_;
	
	my ($id, $seq, $state);
	
	$state = 0;
	$seq = "";
	
	foreach my $line (split(m/\n/, $text))
	{
		if ($state == 0 and $line =~ /^ID\s+(\S+)/)
		{
			$id = $1;
			$state = 1;
		}
		elsif ($state == 1 and substr($line, 0, 2) eq 'SQ')
		{
			$state = 2;
		}
		elsif ($state == 2 and substr($line, 0, 2) ne '//')
		{
			$line =~ s/\s+//g;
			$line =~ s/\d+//g;
			$seq .= $line;
		}
	}
	
	$seq =~ s/(.{60})/$1\n/g;
	
	return $q->pre(">$id\n$seq\n");
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'id' => 'Identification',
		'ac' => 'Accession number',
		'cc' => 'Comments and Notes',
		'dt' => 'Date',
		'de' => 'Description',
		'gn' => 'Gene name',
		'os' => 'Organism species',
		'og' => 'Organelle',
		'oc' => 'Organism classification',
		'ox' => 'Taxonomy cross-reference',
		'ref' => 'Any reference field',
		'dr' => 'Database cross-reference',
		'kw' => 'Keywords',
		'ft' => 'Feature table data',
		'sv' => 'Sequence version',
		'fh' => 'Feature table header',
		'topology'	=> 'Topology (circular or linear)',
		'mt' => 'Molecule type',
		'dc' => 'Data class',
		'td' => 'Taxonomic division',
		'length' => 'Sequence length'
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
