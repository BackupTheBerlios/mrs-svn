#!/opt/local/bin/perl -w
#
# $Id: Format_swiss.pm,v 1.18 2005/09/10 07:14:05 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_swiss;

use Data::Dumper;
use POSIX qw(strftime);
use Time::Local;

our @ISA = "Format";

my %links = (
	'EMBL'	=>	{
		match	=> qr|^(\S+?)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=embl_release|embl_updates&query=ac:$1"}, $1)'
	},

	'UNIPROT'	=> {
		match	=> qr[^(\S+)(?=;)]i,
		result	=> '$q->a({-href=>"$url?db=uniprot&query=ac:$1"}, $1)'
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

	'MIM'		=> {
		match	=> qr|^(\d+)|i,
		result	=> '$q->a({-href=>"$url?db=omim&id=$1"}, $1)'
	},

	'Pfam'		=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=pfamseed|pfama|pfamb&query=ac:$1"}, $1)'
	},
	
	'Ensembl'	=> {
		match	=> qr|^(ENSG\d+)|i,
		result	=> '$q->a({-href=>"http://www.ensembl.org/Homo_sapiens/geneview?gene=$1"}, $1)'
	},

	'InterPro'	=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=interpro&id=$1"}, $1)'
	},

	'PROSITE'	=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=prosite&query=ac:$1"}, $1)'
	},
	
);

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'swiss',
		@_
	};
	my $result = bless $self, "Format_swiss";
	return $result;
}

sub print_ref
{
	my ($q, $ref) = @_;
	my @rr;

	my @rx = split(m/;\s*/, $ref->{rx}) if defined $ref->{rx};
	foreach my $rx (@rx)
	{
		my ($name, $ident) = split(m/=/, $rx);
		
		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?cmd=Retrieve&db=PubMed&list_uids=$ident&dopt=Abstract"}, $ident)
			if ($name eq 'MEDLINE');

		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/utils/qmap.cgi?uid=$ident&form=6&db=m&Dopt=r"}, $ident)
			if ($name eq 'PubMed');
		
		$ident = $q->a({-href=>"http://dx.doi.org/$ident"}, $ident)
			if ($name eq 'DOI');
		
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, $name), $q->td($ident));
	}
	
	push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Position'), $q->td(lc $ref->{rp}))
		if $ref->{rp};
	
	my @rc = split(m/;\s*/, $ref->{rc}) if defined $ref->{rc};
	foreach my $rc (@rc)
	{
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Comment'), $q->td(lc $rc));
	}

	push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Group'), $q->td($ref->{rg}))
		if defined $ref->{rg};
	
	my @l;
	push @l, $ref->{ra};
	push @l, $q->strong($ref->{rt}) if $ref->{rt};
	push @l, $ref->{rl};
	push @l, $q->br;
	push @l, $q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @rr) if scalar @rr;
	
	return join(" ", @l);
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	if (not defined $url or length($url) == 0) {
		my $url = $q->url({-full=>1});
	}

	my (@entry_rows, @name_rows, @keywords_rows, @references_rows, @comments_rows,
		@copyright_rows, @cross_reference_rows, @features_rows, @sequence_rows);
	
	my @entry_info;
	my @name_and_origin;
	my (%ref, @ref_rows, $ref_ix, @refs);
	
	my @lines = split(m/\n/, $text);
	my (@os, @oc, $og, @ox, $os_ix);
	my $kw_ix;
	my @comments;
	
	my $lookahead = shift @lines;
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = shift @lines;
		
		# first block, the entry information
		if ($line =~ /^ID\s+(\S+)/)
		{
			push @entry_rows, $q->Tr($q->th({-colspan=>2}, 'Entry information'));
			push @entry_rows, $q->Tr($q->td({-width=>'20%'}, 'Entry name'), $q->td($q->strong($1)));
		}
		elsif ($line =~ /^AC\s+(.+)/)
		{
			my @acc;

			push @acc, split(m/;\s*/, $1);
			
			while ($lookahead =~ /^AC\s+(.+)/)
			{
				push @acc, split(m/;\s*/, $1);
				$lookahead = shift @lines;
			}

			push @entry_rows, $q->Tr($q->td('Primary accession'), $q->td($q->strong(shift @acc)));
			push @entry_rows, $q->Tr($q->td('Secondary accessions'), $q->td(join(' ', @acc)))
				if (scalar @acc);
		}
		elsif ($line =~ /^DT\s+(\d{2})-([A-Z]{3})-(\d{4}) \(Rel. (\d+), (.+)\)/)
		{
			my %months = ( 'JAN' => 0, 'FEB' => 1, 'MAR' => 2, 'APR' => 3, 'MAY' => 4, 'JUN' => 5, 
				 'JUL' => 6, 'AUG' => 7, 'SEP' => 8, 'OCT' => 9, 'NOV' => 10, 'DEC' => 11);
			
			my $release = $4;
			my $desc = $5;
			my $date = strftime("%B %Y", 0, 0, 0, $1, $months{$2}, $3 - 1900);
			
			push @entry_rows, $q->Tr($q->td($desc), $q->td("Release $release, $date"));
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
			
			my @synonyms;
			my ($includes, $contains);
			
			if ($de =~ /\[Includes: ([^]]+)]/)
			{
				my @inc;
				
				foreach my $i (split(m/;\s*/, $1))
				{
					push @inc, $q->td($i);
				}
				
				$includes = $q->Tr($q->td({-rowspan=>(scalar @inc)}, 'Includes'), shift @inc);
				$includes .= $q->Tr(@inc);
				
				$de =~ s/\[Includes: ([^]]+)]//;
			}

			if ($de =~ /\[Contains: ([^]]+)]/)
			{
				my @inc;
				
				foreach my $i (split(m/;\s*/, $1))
				{
					push @inc, $q->td($i);
				}
				
				$contains = $q->Tr($q->td({-rowspan=>(scalar @inc)}, 'Contains'), shift @inc);
				$contains .= $q->Tr(\@inc);

				$de =~ s/\[Contains: ([^]]+)]//;
			}
			
			while ($de =~ /\(([^)]+)\)/)
			{
				my $s = $1;
				
				if ($s =~ /(EC\s*)(\d+\.\d+\.\d+\.\d+)/)
				{
					$s = $q->a({-href=>"$url?db=enzyme&id=$2"}, $1 . $2);
				}
				
				push @synonyms, $q->td($s);
				
				$de =~ s/\(([^)]+)\)//;
			}
			
			push @name_rows, $q->Tr($q->th({-colspan=>2}, 'Name and origin of the protein'));
			push @name_rows, $q->Tr($q->td('Protein name'), $q->td($de));
			
			if (@synonyms)
			{
				push @name_rows, $q->Tr($q->td({-rowspan=>(scalar @synonyms)}, 'Synonyms'), shift @synonyms);
				push @name_rows, $q->Tr(\@synonyms) if scalar @synonyms;
			}
			
			push @name_rows, $includes if defined $includes;
			push @name_rows, $contains if defined $contains;
		}

		elsif ($line =~ /^GN\s+(.+)/)
		{
			my @gns;
			my $gn = $1;
			my $gnl;
			
			for (;;)
			{
				if ($lookahead =~ /^GN\s+(.+)/ and $1 ne 'and')
				{
					$gn .= " $1";
					$lookahead = shift @lines;
					next;
				}

				my @gf;

				foreach my $p (split(m/;\s*/, $gn))
				{
					if ($p =~ /(.+?)=(.+)/)
					{
						push @gf, $q->Tr($q->td({-class=>'label', -width=>'20%'}, $1), $q->td($2));
					}
				}
				
				push @gns, $q->td($q->div({-class=>'sub_entry'},
					$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @gf)));
				
				if ($lookahead =~ /^GN\s+/)
				{
					die "error here" if (not ($lookahead =~ /^GN\s+and/));
					$lookahead = shift @lines;
					
					$gn = undef;
					
					next;
				}
				last;
			}
			
			push @name_rows, $q->Tr($q->td({-rowspan=>(scalar @gns)}, 'Gene names'), shift @gns);
			push @name_rows, $q->Tr(\@gns) if scalar @gns;
		}
		elsif ($line =~ /^OS\s+(.+)\.?$/)
		{
			my $osl = $1;

			while (not ($line =~ /OS\s+(.+)(,( and)?|\.)/) and $lookahead =~ /^OS\s+(.+)/)
			{
				$osl .= " $1";
				$line = $lookahead;
				$lookahead = shift @lines;
			}
			
			$osl =~ s/(\.|,(\s*and)?)$//;
			
			push @os, $osl;
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
		elsif ($line =~ /^OX\s+NCBI_TaxID=(.+)\;/)
		{
			push @ox, split(m/,\s*/, $1);
		}
		
		# reference part
		
		elsif ($line =~ /^RN\s+\[(\d+)\]/)
		{
			my $nr = $1;

			push @references_rows, $q->Tr($q->th({-colspan=>2}, 'References'))
				unless scalar @references_rows;
			
			if (%ref)
			{
				push @references_rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
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
			$ref{rx} .= "$1 ";
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
		
		# Comment section
		
		elsif ($line =~ /^CC\s+-{10,}$/)
		{
			my $copyright;
			
			while (not $lookahead =~ /^CC\s+-{10,}$/ and $lookahead =~ /^CC\s+(.+)/)
			{
				$copyright .= "$1 ";
				$lookahead = shift @lines;
			}
			
			push @copyright_rows, $q->Tr($q->th({-colspan=>2}, 'Copyright'));
			push @copyright_rows, $q->Tr($q->td({-colspan=>2}, $q->small($copyright)));
			
			$lookahead = shift @lines if $lookahead =~ /^CC\s+-{10,}$/;
		}
		elsif ($line =~ /^CC\s+(.+)/)
		{
			my $cc = $1;

			push @comments_rows, $q->Tr($q->th({-colspan=>2}, 'Comments'))
				unless scalar(@comments_rows);

			if ($cc =~ /^\-\!\-\s+(.+?):\s*(.*)/)
			{
				my $topic = lc $1;
				my $text = $2;
				my ($name, @synonyms, @iso_id, @seq, $note);
				
				if ($topic eq 'alternative products')
				{
					my @ap;
					my ($event, $ni);
					
					while ($lookahead =~ /^CC\s{6,}(.+)/)
					{
						my $cc = $1;
						
						$lookahead = shift @lines;
						
						if (not ($cc =~ /;$/) and $lookahead =~ /^CC\s{6,}(.+)/)
						{
							$cc .= " $1";
							$lookahead = shift @lines;
						}
						
						if ($cc =~ /Event=(.+?);(Named isoforms=(\d+);)?/)
						{
							$event = $1;
							$ni = $3;
						}
						elsif ($cc =~ /Name=(.+?);\s*(Synonyms=(.+);)?/)
						{
							if (defined $name)
							{
								push @ap, $q->Tr(
									$q->td($name),
									$q->td(join(', ', @synonyms)),
									$q->td(join(', ', @iso_id)),
									$q->td(join(', ', @seq)),
									$q->td($note));
									
								undef @synonyms;
								undef @iso_id;
								undef @seq;
								undef $note;
							}
							
							$name = $1;
							push @synonyms, split(m/,\s*/, $3);
						}
						elsif ($cc =~ /IsoId=(.+?);\s*(Sequence=(.+);)?/)
						{
							push @iso_id, split(m/,\s*/, $1);
							push @seq, split(m/,\s*/, $3);
						}
						elsif ($cc =~ /Comment=(.+?);/)
						{
							$event .= " ($1)";
						}
						elsif ($cc =~ /Note=(.+?);/)
						{
							$note .= $1;
						}
					}

					if (defined $name)
					{
						push @ap, $q->Tr(
							$q->td($name),
							$q->td(join(', ', @synonyms)),
							$q->td(join(', ', @iso_id)),
							$q->td(join(', ', @seq)),
							$q->td($note));
					}
					
					$text = $event;
					if (scalar @ap)
					{
						$text .= $q->br;
						$text .= $q->div({-class=>'sub_entry'},
							$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'},
							$q->Tr(
								$q->th('Name'),
								$q->th('Synonyms'),
								$q->th('IsoId'),
								$q->th('Sequence'),
								$q->th('Note')
							), @ap));
					}
				}
				else
				{
					while ($lookahead =~ /^CC\s{6,}(.+)/)
					{
						$text .= "$1\n";
						$lookahead = shift @lines;
					}
				}
				
				push @comments_rows, $q->Tr($q->td($topic), $q->td($text));
			}
		}
		elsif ($line =~ /^DR\s+(.+)/)
		{
			my @dr;
			my $last_db = "";

			push @cross_reference_rows, $q->Tr($q->th({-colspan=>2}, 'Cross-references'));
			
			for (;;)
			{
				my ($db, $ids) = split(m/;\s*/, substr($line, 5), 2);

				if ($db ne $last_db and scalar @dr)
				{
					
					push @cross_reference_rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
					push @cross_reference_rows, $q->Tr(\@dr) if scalar @dr;
					
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

			push @cross_reference_rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
			push @cross_reference_rows, $q->Tr(\@dr) if scalar @dr;
		}
		
		# Keywords
		
		elsif ($line =~ /^KW\s+(.+)/)
		{
			my @kw = split(m/;\s*/, $1);
			
			while ($lookahead =~ /^KW\s+(.+)/)
			{
				push @kw, split(m/;\s*/, $1);
				$lookahead = shift @lines;
			}

			my @linked_kw;
			foreach my $kw (@kw)
			{
				$kw =~ s/\.$//;
				my $kw_url = $url . "?db=uniprot&query=";
				foreach my $kww (split(m/\s/, $kw))
				{
					$kw_url .= "kw:$kww ";
				}
				push @linked_kw, $q->a({-href=>$kw_url}, $kw);
			}

			push @keywords_rows, $q->Tr($q->td('Keywords'), $q->td(join('; ', @linked_kw)));
		}
		
		# Feature table
		
		elsif ($line =~ /^FT/)
		{
			my @ft;
			
			push @ft, $q->Tr(
				$q->th({-width=>'10%'}, 'Key'),
				$q->th({-width=>'5%', -style=>'text-align:right'}, 'From'),
				$q->th({-width=>'5%', -style=>'text-align:right'}, 'To'),
				$q->th({-width=>'5%', -style=>'text-align:right'}, 'Length'),
				$q->th({-width=>'75%'}, 'Description'));
			
			for (;;)
			{
				my $key = substr($line, 5, 8);		$key =~ s/\s+$//;
				my $from = substr($line, 14, 6);	$from =~ s/\s+$//;
				my $to = substr($line, 21, 6);		$to =~ s/\s+$//;
				my $desc = substr($line, 34, 41);
				
				while ($lookahead =~ /^FT\s{10}/)
				{
					my $s = substr($lookahead, 34, 41);
					
					$desc .= $q->br if ($s =~ m|^/|);
					
					$desc .= " $s";
					$lookahead = shift @lines;
				}
				
				my $length = $to - $from + 1;
#				$length = '-' if $length == 0;
				
				push @ft, $q->Tr(
					$q->td(lc $key),
					$q->td({-style=>'text-align:right'}, $from),
					$q->td({-style=>'text-align:right'}, $to),
					$q->td({-style=>'text-align:right'}, $length),
					$q->td($desc));
				
				last unless $lookahead =~ /^FT/;
				
				$line = $lookahead;
				$lookahead = shift @lines;
			}
			
			push @features_rows, $q->Tr($q->th({-colspan=>2}, 'Features'));
			push @features_rows, $q->Tr($q->td({-colspan=>2}, $q->div({-class=>'sub_entry'},
					$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @ft))));
		}
		
		elsif ($line =~ /^SQ\s*SEQUENCE\s*(\d+)\s*AA;\s*(\d+)\s*MW;\s*(.+)\s*CRC64;/)
		{
			my $aa = $1;
			my $mw = $2;
			my $crc = $3;
			
			push @sequence_rows, $q->Tr($q->th({-colspan=>2}, 'Sequence information'));
			push @sequence_rows, $q->Tr($q->td({-colspan=>2}, 
"Length: <strong>$aa aa</strong>, molecular weight <strong>$mw Da</strong>, CRC64 checksum <strong>$crc</strong>"));

			my $seq;
			while (not ($lookahead =~ m|//|))
			{
				$lookahead =~ s/^\s+//;
				
				$seq .= "$lookahead\n";
				$lookahead = shift @lines;
			}
			
			push @sequence_rows, $q->Tr($q->td({-colspan=>2}, $q->pre($seq)));
		}
	}

	if (@os)
	{
		die "mismatch OS " . scalar @os . " OX " . scalar @ox . " (@os)" if scalar @os != scalar @ox;
		
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
		
		push @name_rows, $q->Tr($q->td({-rowspan=>(scalar @osr)}, 'From'), shift @osr);
		push @name_rows, $q->Tr(\@osr) if scalar @osr;
		
		push @name_rows, $q->Tr($q->td('Encoded on'), $q->td($og)) if $og;
		
		push @name_rows, $q->Tr($q->td('Taxonomy'), $q->td(
			join("; ", map { $q->a({-href=>"$url?db=taxonomy&query=$_"}, $_) } @oc)));
	}

	if (%ref)
	{
		push @references_rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
			&print_ref($q, \%ref))));
	}

	my @rows;
	push @rows, @entry_rows if scalar @entry_rows;
	push @rows, @name_rows if scalar @name_rows;
	push @rows, @keywords_rows if scalar @keywords_rows;
	push @rows, @references_rows if scalar @references_rows;
	push @rows, @comments_rows if scalar @comments_rows;
	push @rows, @copyright_rows if scalar @copyright_rows;
	push @rows, @cross_reference_rows if scalar @cross_reference_rows;
	push @rows, @features_rows if scalar @features_rows;
	push @rows, @sequence_rows if scalar @sequence_rows;

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
			$seq .= $line;
		}
	}
	
	$seq =~ s/(.{60})/$1\n/g;
	
	return ">$id\n$seq\n";
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
		'crc64' => 'The CRC64 checksum for the sequence',
		'length' => 'The length of the sequence',
		'mw' => 'Molecular weight',
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
