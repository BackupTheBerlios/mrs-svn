#!/usr/pkg/bin/perl -w

use strict;
use MRS;
use MRSCGI qw/-no_xhtml/;
use CGI::Cookie;
use CGI::Carp qw(fatalsToBrowser);
use Data::UUID;
use Data::Dumper;
use POSIX qw(strftime);
use XML::SAX;
use XML::SAX::Expat;
use MRSCommon;
use Format;

my $q = new MRSCGI(script=>'result.cgi');
my $ug = new Data::UUID;

my %cookies = fetch CGI::Cookie;

XML::SAX->add_parser(q(XML::SAX::Expat));
my $factory = XML::SAX::ParserFactory->new();

our @dbs;
our $mrs_data;
our $mrs_tmp_dir;

my $settings = 'mrs.conf';
unless (my $return = do $settings)
{
	warn "couldn't parse $settings: $@" if $@;
	warn "couldn't do $settings: $!" unless defined $return;
	warn "couldn't run $settings" unless $return;
}

my $work_dir = $mrs_tmp_dir;
my $base_url = $q->base_url;

my @dbIds = map { $_->{id} } @dbs;
my %dbLabels = map { $_->{id} => $_->{name} } @dbs;
my %dbFilters = map { $_->{id} => $_->{filter} } @dbs;
my %dbInfo = map { $_->{id} => $_ } @dbs;

foreach my $dbid (@dbIds)
{
	if ($dbid =~ /\|/)
	{
		foreach my $i (split(m/\|/, $dbid))
		{
			$dbFilters{$i} = $dbFilters{$dbid};
		}
	}
}

$ENV{MRS_DATA_DIR} = $mrs_data;

my $kGraphicWidth = 100;

&main();
exit;

my $DEFAULT_MARKER = "…";

sub Filter
{
	my $db = shift;
	
	my $result = $dbFilters{$db};
	if (not defined $result)
	{
		foreach my $db (split(m/\|/, $db))
		{
			$result = $dbFilters{$db};
			last if defined $result;
		}
	}
	
	return $result;
}

sub truncstr
{
    my ($string, $cutoff, $marker) = @_;

    $marker = ($DEFAULT_MARKER or ""), unless (defined($marker));

    croak "continued symbol is longer than the cutoff length"
      if (length($marker) > $cutoff);

    if (length($string) > $cutoff)
    {
		$string = (substr($string, 0, $cutoff - length($marker)) or "") . $marker;
    }

    return $string;
}

sub getCookie
{
	my ($param, $value, $expire) = @_;
	
	my $cookie = $cookies{$param};
	$cookie = new CGI::Cookie(-name => $param, -value => $value)
		unless defined $cookie;
	$cookie->expires('+1y') unless $expire;
	
	return $cookie;
}

sub getUrl()
{
	my ($db, $id) = @_;
	
	my $mrs_url = "$base_url/cgi-bin/mrs.cgi?db=%s&id=%s";

	return $q->a({-href=>sprintf($mrs_url, $db, $id)}, $id);
}

sub printNavBar
{
	my ($first, $count, $total, $name, $base_url, $db) = @_;
	
	my $last = $first + $count;
	$last = $total + 1 if ($last > $total + 1);

	my @btns;

	my $display_last = $last - 1;
	
	push @btns, $q->li("$name $first-$display_last of $total");
	push @btns, $q->li({-style=>'width:60px'}, "&nbsp;");

	if ($first <= 1)
	{
		push @btns, $q->li({-class=>'disabled'}, "<<");
		push @btns, $q->li({-class=>'disabled'}, "<");
	}
	else
	{
		my $url = sprintf($base_url, 1, $count);
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>$url}, "<<"));	

		my $prev = $first - $count;
		$prev = 1 if $prev <= 1;
		$url = sprintf($base_url, $prev, $count);
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>$url}, "<"));	
	}

	my $next = $first + $count;
	$last = $total - $count + 1;
#	$next = $last if $next > $last;

	if ($first >= $last)
	{
		push @btns, $q->li({-class=>'disabled'}, ">");
		push @btns, $q->li({-class=>'disabled'}, ">>");
	}
	else
	{
		my $url = sprintf($base_url, $next, $count);
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>$url}, ">"));

		$last = $total - $count + 1;
		$url = sprintf($base_url, $last, $count);
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>$url}, ">>"));
	}

#	push @btns, $q->li({-style=>'width:300px'}, "&nbsp;");
#
	my $url = $q->base_url;
	
	return
		$q->div({-class=>'nav', -style=>'position:relative; height: 20px;'},
			$q->ul(@btns),
			$q->div({-style=>'position:absolute; right: 2px; top: 2px;'},
				$q->button(
					-name=>'clustalw',
					-value=>"ClustalW",
					-class=>'submit',
					-onClick=>"doSave('$url/cgi-bin/mrs-clustalw.cgi', '$db');"
				)		
			)
		);
}

sub printHeader
{
	my ($query_name, $q, $query_id, $query_desc) = @_;
	my ($result, @options, %options);
	
	my $params = "$query_name.params-mrs";
	$params = "$query_name.params-ncbi" unless -f $params;
	
	open PARAMS, "<$params";
	foreach my $option (grep { m/^[^.]/ } <PARAMS>)
	{
		chomp($option);
		my ($key, $value) = split(m/=/, $option);
		$options{$key} = $value;
	}
	close PARAMS;
	
	my $input;
	{
		local($/) = undef;
		open FA, "<$query_name.fa";
		$input = <FA>;
		close FA;
	}
	$input =~ s/>/\&gt;/g;
	$input = $q->div({-id => 'input_box' }, $q->pre($input));

	push @options, $q->Tr($q->td('program'),	$q->td('-p'),	$q->td($options{program}))
		if ($options{program});
	push @options, $q->Tr($q->td('datatbanks'),	$q->td('-d'),	$q->td($options{dbs}))
		if ($options{dbs});
	if ($options{limit} eq 'on')
	{
		push @options, $q->Tr($q->td('limit output'), $q->td('-v &amp; -b'),	$q->td($options{limit_nr}));
	}
	push @options, $q->Tr($q->td('filter'),		$q->td('-F'),	$q->td($options{filter} ? "T" : "F"))
		if ($options{filter});
	push @options, $q->Tr($q->td('matrix'),		$q->td('-M'),	$q->td($options{matrix}))
		if ($options{matrix});
	if ($options{wordsize} and $options{wordsize} ne 'Default')
	{
		push @options, $q->Tr($q->td('wordsize'),	$q->td('-W'),	$q->td($options{wordsize}));
	}
	push @options, $q->Tr($q->td('expect'),		$q->td('-e'),	$q->td($options{expect}))
		if ($options{expect});
	if ($options{gapped} eq 'on')
	{
		if ($options{gapopen} ne 'Default')
		{
			push @options, $q->Tr($q->td('gap open cost'), $q->td('-G'),	$q->td($options{gapopen}));
		}
		
		if ($options{gapextend} ne 'Default')
		{
			push @options, $q->Tr($q->td('gap extend cost'), $q->td('-E'),	$q->td($options{gapextend}));
		}
	}
	elsif ($options{gapped})
	{
		push @options, $q->Tr($q->td('gapped'),	$q->td('-g'),	$q->td('F'));
	}
	push @options, $q->Tr($q->td('input'),		$q->td('-i'),	$q->td($input));
	
	my $header = "Result for ";
	$header .= $query_id if $query_id;
	$header .= "'$query_desc' " if $query_desc;
	$header .= $q->span(
		{
			-class => 'switch',
			-onClick => 'showBox("jp", "job parameters")',
			-id => 'jp'
		},
		'Show job parameters'
	);

	my $repeat_url = "$base_url/cgi-bin/mrs-blast.cgi?restore=$query_name";
	$repeat_url =~ s/mrs-blast/blast/ unless $params eq "$query_name.params-mrs";

	$header .= " or " . $q->a({-href => $repeat_url }, "Repeat this query");
	
	$result = $q->div({-class => 'box' },
			$q->ul(
#				$q->li($q->pre(Dumper(\%options))),
				$q->li($header),
				$q->li(
					$q->div({-class=>'list', -id => 'jp_text', -style => 'display:none' },
						$q->table({-cellspacing=>0, -cellpadding=>0, -id=>'Options'}, @options)
					)
				)
			)
		);
	
	return $result;
}

sub printResult
{
	my ($hits, $options, $query_id, $query_desc, $query_len, $hit_count, $query_name) = @_;
	
	my $result;

	$result = $q->br;
	$result .= 
		&printNavBar(@{$hits}[0]->{hit_nr}, scalar @{$hits}, $hit_count, "Hits",
			"$base_url/cgi-bin/result.cgi?query=$query_name&first=%i&count=%i",
			$options->{dbs});
	
	my $mrs_db = new MRS::MDatabank($options->{dbs})
		or die "MRS databank does not exist?";
	
	my @rows;

	push @rows, $q->Tr(
		$q->th("Nr"),
		$q->th("Accession"),
		$q->th("Coverage"),
		$q->th("Description"),
		$q->th("Bitscore"),
		$q->th("E-value"),
		$q->th
	);
	
	for (my $i = 0; $i < scalar @{$hits}; ++$i)
	{
		my $row = (@{$hits})[$i];
		
		my $acc = $row->{hit_acc};
		my $hit_id = "${acc}_$i";
				
		my ($mqf, $mqt, $bitscore, $evalue, $bin);
		
		my @hsps;
		
		push @hsps, $q->Tr(
			$q->th("Hsp nr"),
			$q->th("Alignment"),
			$q->th("Score"),
			$q->th("Bitscore"),
			$q->th("E-value"),
			$q->th("Identity"),
			$q->th("Similarity"),
			$q->th("Gaps")
		);
		
		foreach my $hsp (@{$row->{hit_hsps}})
		{
			$mqf = $hsp->{hsp_query_from} unless defined $mqf;
			$mqt = $hsp->{hsp_query_to} unless defined $mqt;
			$bitscore = $hsp->{hsp_bit_score} unless defined $bitscore;
			$evalue = $hsp->{hsp_evalue} unless defined $evalue;
#			$bin = 
#				((($hsp->{hsp_positive} * 4) / $hsp->{hsp_align_len}) +
#				 (($hsp->{hsp_align_len} * 4) / $query_len)) / 2
#				unless defined $bin;
			$bin = 
				((($hsp->{hsp_positive} * 4) / $hsp->{hsp_align_len}))
				unless defined $bin;
			
			my $qf = $hsp->{hsp_query_from} - 1;
			my $qt = $hsp->{hsp_query_to} - 1;
			
			($qt, $qf) = ($qf, $qt) if ($qf > $qt);
			
			my $sf = $hsp->{hsp_hit_from} - 1;
			my $st = $hsp->{hsp_hit_to} - 1;
			
			($st, $sf) = ($sf, $st) if ($sf > $st);
	
			my ($ql1, $ql2, $ql3, $ql4);
			my ($sl1, $sl2, $sl3, $sl4);
	
			my $total_length = $hsp->{hsp_align_len};
			
			my $before = ($qf > $sf) ? $qf : $sf;
			my $after = ($query_len - $qt > $row->{hit_len} - $st) ?
				$query_len - $qt : $row->{hit_len} - $st;
	
			$total_length += $before + $after;
			my $factor = 150 / $total_length;

			$ql1 = 0;
			$ql1 = int($factor * ($before - $qf)) if ($qf < $before);
			$ql2 = int($factor * $qf);
			$ql3 = int($factor * $hsp->{hsp_align_len});
			$ql4 = int($factor * ($query_len - $qt));
			
			my $image = "<div class='alignment'>";
			$image .= "<img src='../img/pixel-a.png' height='5' width='$ql1'>" if ($ql1 > 0);
			$image .= "<img src='../img/pixel-d.png' height='5' width='$ql2'>" if ($ql2 > 0);
			$image .= "<img src='../img/pixel-e.png' height='5' width='$ql3'>" if ($ql3 > 0);
			$image .= "<img src='../img/pixel-d.png' height='5' width='$ql4'>" if ($ql4 > 0);
			$image .= "\n";
			
			$sl1 = 0;
			$sl1 = int($factor * ($before - $sf)) if ($sf < $before);
			$sl2 = int($factor * $sf);
			$sl3 = int($factor * $hsp->{hsp_align_len});
			$sl4 = int($factor * ($row->{hit_len} - $st));

			$image .= "<img src='../img/pixel-a.png' height='5' width='$sl1'>" if ($sl1 > 0);
			$image .= "<img src='../img/pixel-b.png' height='5' width='$sl2'>" if ($sl2 > 0);
			$image .= "<img src='../img/pixel-c.png' height='5' width='$sl3'>" if ($sl3 > 0);
			$image .= "<img src='../img/pixel-b.png' height='5' width='$sl4'>" if ($sl4 > 0);
			$image .= "</div>";
			
			#
			# And now the multiple alignment
			#
			
			my $qseq = $hsp->{hsp_qseq};
			my $hseq = $hsp->{hsp_hseq};
			my $midl = $hsp->{hsp_midline};
			
			my $fqseq;
			$fqseq = MRS::DUST($qseq) if ($options->{filter} eq 'D');
			$fqseq = MRS::SEG($qseq) if ($options->{filter} eq 'S');
			$fqseq = $qseq if not defined $fqseq;
			
			my ($n, $hqf, $hhf) = (0, $hsp->{hsp_query_from}, $hsp->{hsp_hit_from});
			
			my $alignment;
			
			while ($n < length($qseq))
			{
				#start with a query line
				
				my $k = length($qseq) - $n;
				$k = 60 if ($k > 60);
				
				my $kq = $k;
				$kq = length($qseq) if ($kq > length($qseq));
				
				my $nq = $hqf;
				my $nh = $hhf;
				
				# count the dashes in this part
				$nq += substr($qseq, $n + 1, $kq) =~ tr/-//c;
				$nh += substr($hseq, $n + 1, $kq) =~ tr/-//c;

				my $qal = sprintf("Q: %6.f  ", $hqf);
				my $mid = "           ";
				my $hal = sprintf("S: %6.f  ", $hhf);;
				
				if (length($fqseq) > 0)
				{
					my $colored = 0;
					
					for (my $c = 0; $c < $kq; ++$c)
					{
						my $ch = substr($fqseq, $n + $c, 1);
						
						if ($ch eq "X") {
							$ch = substr($qseq, $n + $c, 1);
							if (not $colored) {
								$qal .= '<span class="masked1">';
								$mid .= '<span class="masked2">';
							}
							$colored = 1;
						}
						else {
							if ($colored) {
								$qal .= "</span>";
								$mid .= "</span>";
							}
							$colored = 0;
						}

						$qal .= $ch;
						$mid .= substr($midl, $n + $c, 1);
					}
					
					if ($colored) {
						$qal .= "</span>";
						$mid .= "</span>";
					}
				}
				else {
					$qal .= substr($qseq, $n, $kq);
					$mid .= substr($midl, $n, $kq);
				}

				$hal .= substr($hseq, $n, $kq);

				$hqf = $nq;
				$hhf = $nh;

				$n += $k;
				
				if ($n < length($qseq))
				{
					--$nq;
					--$nh;
				}

				$qal .= sprintf("  %d\n", $nq);
				$mid .= "\n";
				$hal .= sprintf("  %d\n", $nh);

				$alignment .= $qal;
				$alignment .= $mid;
				$alignment .= $hal;
				$alignment .= "\n";
			}
			
			push @hsps, $q->Tr(
				{
					-id => "tr_$hit_id:$hsp->{hsp_nr}",
					-class=>'list_item',
					-onClick=>"show(\'$hit_id:$hsp->{hsp_nr}\')",
					-onMouseOver=>"enterItem('tr_$hit_id:$hsp->{hsp_nr}');",
					-onMouseOut=>"leaveItem('tr_$hit_id:$hsp->{hsp_nr}');",
					-style=>"text-align:right;"
				},
				$q->td($hsp->{hsp_nr}),
				$q->td({-style=>"text-align:left"}, $image),
				$q->td(sprintf("%.0f", $hsp->{hsp_score})),
				$q->td(sprintf("%.0f", $hsp->{hsp_bit_score})),
				$q->td(sprintf("%.2g", $hsp->{hsp_evalue})),
				$q->td(sprintf("%d/%d (%.0f%%)", $hsp->{hsp_identity},
					$hsp->{hsp_align_len}, ($hsp->{hsp_identity} * 100) / $hsp->{hsp_align_len})), #/
				$q->td(sprintf("%d/%d (%.0f%%)", $hsp->{hsp_positive}, $hsp->{hsp_align_len},
					($hsp->{hsp_positive} * 100) / $hsp->{hsp_align_len})), #/
				$q->td("$hsp->{hsp_gaps}/$hsp->{hsp_align_len}")
				);

			push @hsps, $q->Tr({-id=>"$hit_id:$hsp->{hsp_nr}", -style=>"display:none;"},
				$q->td({-colspan=>7}, $q->pre($alignment)));
		}
		
		($mqt, $mqf) = ($mqf, $mqt) if ($mqf > $mqt);

		my $l1 = int(($mqf * $kGraphicWidth) / $query_len); #/
		my $l2 = int((($mqt - $mqf) * $kGraphicWidth) / $query_len); #/
		my $l3 = $kGraphicWidth - $l1 - $l2;
		
		if ($bin >= 3.25)		{ $bin = 1; }
		elsif ($bin >= 2.5)		{ $bin = 2; }
		elsif ($bin >= 1.75)	{ $bin = 3; }
		else					{ $bin = 4; }
		
		my $record;
		eval {
			$record = $mrs_db->Get($row->{hit_id});
		};
		
		push @rows, $q->Tr(
			{
				-id=>"tr_$hit_id",
				-class=>'list_item',
				-onMouseOver=>"enterItem('tr_$hit_id');",
				-onMouseOut=>"leaveItem('tr_$hit_id');"
			},
			$q->td({-onClick=>"show('$hit_id')", -style => 'text-align:right' }, $q->strong($row->{hit_nr})),
			$q->td(&getUrl($options->{dbs}, $row->{hit_id})),
			$q->td({-onClick=>"show('$hit_id')"},
				$q->div({-class=>"alignment"},
					"<img src='../img/pixel-d1.png' height='4' width='$l1'>".
					"<img src='../img/pixel-e$bin.png' height='4' width='$l2'>".
					"<img src='../img/pixel-d1.png' height='4' width='$l3'>"
				)
			),
#			$q->td({-onClick=>"show('$hit_id')"}, $row->{hit_def}),
			$q->td({-onClick=>"show('$hit_id')"}, Format::desc(&Filter($options->{dbs}), $q, $record)),
			$q->td({-onClick=>"show('$hit_id')", -style=>"text-align:right"}, sprintf("%.0f", $bitscore)),
			$q->td({-onClick=>"show('$hit_id')", -style=>"text-align:right"}, sprintf("%.2g", $evalue)),
			$q->td(
				$q->checkbox(
					-id=>$acc,
					-name=>$acc,
					-value=>'on',
					-label=>'',
					-onMouseDown=>'return doMouseDownOnCheckbox(event);',
					-onClick=>"return doClickSelect(\'$acc\')"
				)
			));
		
		push @rows, $q->Tr({-id=>$hit_id, -style=>"display:none"},
			$q->td(""),
			$q->td({-colspan=>4},
				$q->table({-cellspacing=>0, -cellpadding=>0}, @hsps))
			);
	}
	
	$result .=
		$q->div({-class=>'list', -id=>'list'},
			$q->table({-cellspacing=>0, -cellpadding=>0, -id=>'Hits'}, @rows));
	
	return $result;
}

sub printResultForQuery($)
{
	my $query_name = shift;
	
	my $error_box;
	
	if (-s "$query_name.err")	#"
	{
		my $err;
		local($/) = undef;
		open ERR, "<$query_name.err";
		$err = <ERR>;
		close ERR;
		
		$error_box = $q->div({-class => 'box' },
			$q->ul(
				$q->li(
					'The blastall program returned errors:',
					$q->span({-class => 'switch', -onClick => 'showBox("err", "errors")', -id => 'err' },
						'Show errors'
					)
				),
				$q->li({-style => 'display:none', -id => 'err_text' },
					$q->pre($err)
				)
			)
		);
	}

	my $first = $q->param('first');			$first = 1 unless defined $first;
	my $count = $q->param('count');			$count = 20 unless defined $count;
	my $nr_of_hits = $q->param('hits');		$nr_of_hits = 20 unless defined $nr_of_hits;
	my $nr_of_hsps = $q->param('hsps');		$nr_of_hsps = 10 unless defined $nr_of_hsps;
	
	my $parser = $factory->parser(
		Handler => BlastXMLHandler->new(
			$query_name,
			first			=> $first,
			count			=> $count,
			max_hsp_count	=> $nr_of_hsps,
			max_hit_count	=> $nr_of_hits
		)
	);

	my $result;

	eval
	{
		$result = &printHeader($query_name, $q, $parser->{Handler}->{query_id},
			$parser->{Handler}->{'BlastOutput_query-def'});
		
		$parser->parse_uri("$query_name.out") or die "Could not parse file $query_name: $!";
		
		my $hits = $parser->{Handler}->{hits};
		
		if (defined $hits and scalar(@{$hits}))
		{
			$result .= &printResult(
				$hits,
				$parser->{Handler}->{options},
				$parser->{Handler}->{query_id},
				$parser->{Handler}->{'BlastOutput_query-def'},
				$parser->{Handler}->{'BlastOutput_query-len'},
				$parser->{Handler}->{hit_count},
				$query_name
			);
		}
		else
		{
			$result .= $q->p("blastall output did not contain valid data");
		}
	};
	
	if ($@)
	{
		$result .= $q->p("blastall did not return valid output: $@");
	}

	my $copyright = $parser->{Handler}->{BlastOutput_reference};
	$copyright =~ s/~//g;
	
	$result .= $q->p($q->small($copyright));
	$result .= $error_box if ($error_box);
	
	return ($result, 0);
}

sub printList
{
	my @rows;
	
	push @rows, $q->Tr(
		$q->th('Nr'),
		$q->th('ID'),
		$q->th('Desc'),
		$q->th('Db'),
		$q->th('Best e-Value'),
		$q->th('Nr of Hits'),
	);
	
	my $nr = 1;
	my $refresh = 0;
	
	while (1)
	{
		my $name = sprintf("query_%4.4d", $nr);
		last unless -s "$name.csh"; #"
		
		if (not -f "$name.digest")
		{
			eval
			{
				open FA, "<$name.fa";
				my $first_line = <FA>;
				close FA;
				
				my ($def, $id);
				if ($first_line =~ /^>(\S+)(\s+(.+))?/)
				{
					$id = $1;
					$def = $3;
				}
				
				my $parser = $factory->parser(
					Handler => BlastXMLHandler->new(
						$name,
						first			=> 1,
						count			=> 1,
						max_hsp_count	=> 1,
						max_hit_count	=> 1
					)
				);

				$parser->parse_uri("$name.out") or die "Could not parse file $name: $!";

				open DIGEST, ">$name.digest";
				
				if (scalar @{$parser->{Handler}->{hits}})
				{
					my $hit = (@{$parser->{Handler}->{hits}})[0];
					my $db = $parser->{Handler}->{options}->{dbs};
					my $hsp = (@{$hit->{hit_hsps}})[0];
					
					print DIGEST
						$id,											"\t",
						$def,											"\t",
						$db,											"\t",
						$parser->{Handler}->{hit_count},				"\t",
						$hsp->{hsp_evalue},								"\n";
				}

				close DIGEST;
			}
		}
		
		if (-s "$name.digest") #"
		{
			open DIGEST, "<$name.digest";
			my @v = split(m/\t/, <DIGEST>);
			close DIGEST;
			
			push @rows, $q->Tr({
					-id => "q_$nr",
					-onClick => "showQuery('$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOver => "enterItem('q_$nr', '$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOut => "leaveItem('q_$nr');"
				},
				$q->td({-style => 'text-align:right' }, $nr),
				$q->td($v[0]),
				$q->td(&truncstr($v[1], 40)),
				$q->td($v[2]),
				$q->td({-style => 'text-align:right' }, $v[4]),
				$q->td({-style => 'text-align:right' }, $v[3])
			);
		}
		elsif (-z "$name.digest") #"	empty
		{
			push @rows, $q->Tr({
					-id => "q_$nr",
					-onClick => "showQuery('$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOver => "enterItem('q_$nr', '$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOut => "leaveItem('q_$nr');"
				},
				$q->td({-style => 'text-align:right' }, $nr),
				$q->td("No hits found"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;")
			);
		}
		elsif (-s "$name.out" or -s "$name.err") #"
		{
			push @rows, $q->Tr({
					-id => "q_$nr",
					-onClick => "showQuery('$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOver => "return enterItem('q_$nr', '$base_url/cgi-bin/result.cgi?query=$name');",
					-onMouseOut => "leaveItem('q_$nr');"
				},
				$q->td({-style => 'text-align:right' }, $nr),
				$q->td("No valid output in blast result"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;")
			);
		}
		else
		{
			$refresh = 1;
			push @rows, $q->Tr(
				$q->td({-style => 'text-align:right' }, $nr),
				$q->td("Working…"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;"),
				$q->td({-style => 'text-align:right' }, "&nbsp;")
			);
		}
		
		++$nr;
	}
	
	my $result = $q->p("Here you find the list of submitted blast jobs. ");
	$result .= $q->div({-class=>'list'},
			$q->table({-cellspacing=>0, -cellpadding=>0}, @rows)
		);
	
	return ($result, $refresh);
}

sub main()
{
	my $session_id = $q->param('session_id');
	my $session_cookie	= getCookie('session_id', $session_id, 1);

	if (not defined $session_id)
	{
		$session_id = $session_cookie->value;

		if (not defined $session_id)
		{
			$session_id = $ug->create_str() unless defined $session_id;
			$session_cookie->value($session_id);
		}
	}
	elsif ($session_id ne $session_cookie->value)
	{
		$session_cookie->value($session_id);
	}
	
	Format::init();	# needed here since that function looks in the current working directory for format plugins...

	$work_dir .= $session_id;
	if (not -d $work_dir)
	{
		mkdir($work_dir) or die "Could not create working directory $work_dir: $!";
	}
	chdir($work_dir) or die "Could not change directory to $work_dir: $!";
	
	my ($content, $refresh);

	if ($q->param('query'))
	{
		($content, $refresh) = &printResultForQuery($q->param('query'));
	}
	else
	{
		($content, $refresh) = &printList();
	}

	my %header_options = ( -charset=>'utf-8' );
	$header_options{-cookie} = $session_cookie;
	$header_options{-refresh} = 5 if $refresh;

	print
		$q->header(\%header_options),
		$q->start_html({
			-title=>'Blast result page',
			-script=>[
				{'src'=>"$base_url/result.js"},
				{'src'=>"$base_url/mrs.js"}
			],
			-style=>{'src'=>"$base_url/mrs_style.css"}
		}),
		$q->menu,
		$q->div({-id=>'main'}, $content),
		$q->end_html;
}

# -----
#
# The XML parser
#
# -----

package BlastXMLHandler;
use base qw(XML::SAX::Base);

sub new
{
	my ($invocant, $name) = @_;
	my $self =
	{
		query_name => $name,
		param_section => 0,
		hit_count => 0,
		hsp_count => 0,
		hits => [],
		options => {},
		@_
	};
	bless($self, "BlastXMLHandler");
	
	return $self;
}

sub storeHit
{
	my ($self) = @_;

	if ($self->{Hit_num} >= $self->{first} and $self->{Hit_num} < $self->{first} + $self->{count})
	{
		my %hit;
		
		$hit{hit_nr} = $self->{Hit_num};
		$hit{hit_len} = $self->{Hit_len};
		$hit{hit_acc} = $self->{Hit_accession};
		$hit{hit_id} = $self->{Hit_id};
		
		$hit{hit_id} =~ s/^lcl\|//;
		$hit{hit_id} =~ s/^[^:]+://;
		
		$hit{hit_def} = $self->{Hit_def};
		$hit{hit_hsps} = ();
		
		push @{$self->{hits}}, \%hit;
		
		$self->{hsp_count} = 0;
	}
}

sub storeHsp
{
	my ($self) = @_;

#	if ($self->{hit_count} < $self->{max_hit_count} and $self->{hsp_count} < $self->{max_hsp_count})
	if ($self->{Hit_num} >= $self->{first} and $self->{Hit_num} < $self->{first} + $self->{count})
	{
		my %hsp;
		
		$hsp{hsp_nr} = $self->{Hsp_num};
		$hsp{hsp_score} = $self->{'Hsp_score'};
		$hsp{hsp_bit_score} = $self->{'Hsp_bit-score'};
		$hsp{hsp_evalue} = $self->{Hsp_evalue};
		$hsp{hsp_query_from} = $self->{'Hsp_query-from'};
		$hsp{hsp_query_to} = $self->{'Hsp_query-to'};
		$hsp{hsp_hit_from} = $self->{'Hsp_hit-from'};
		$hsp{hsp_hit_to} = $self->{'Hsp_hit-to'};
		$hsp{hsp_query_frame} = $self->{'Hsp_query-frame'};
		$hsp{hsp_hit_frame} = $self->{'Hsp_hit-frame'};
		$hsp{hsp_identity} = $self->{Hsp_identity};
		$hsp{hsp_positive} = $self->{Hsp_positive};
		$hsp{hsp_gaps} = $self->{Hsp_gaps};
		$hsp{hsp_align_len} = $self->{'Hsp_align-len'};
		$hsp{hsp_qseq} = $self->{Hsp_qseq};
		$hsp{hsp_midline} = $self->{Hsp_midline};
		$hsp{hsp_hseq} = $self->{Hsp_hseq};
		
		my $hits = $self->{hits};
		
		push @{(@{$hits}[scalar(@{$hits}) - 1])->{hit_hsps}}, \%hsp;
	}
}

sub start_element
{
	my ($self, $el) = @_;
	
	my $name = $el->{LocalName};
	$self->{BlastElement} = $name;
	$self->{Data} = "";
	
	if ($name eq "Hit")
	{
		++$self->{hit_count};
		$self->{hitStored} = 0;
	}
	elsif ($name eq "Hsp" and not $self->{hitStored})
	{
		$self->storeHit();
		$self->{hitStored} = 1;
	}
}

sub end_element
{
	my ($self, $el) = @_;
	
	my $name = $el->{LocalName};
	$self->{$name} = $self->{Data};
	
	if ($name eq "Hsp")
	{
		$self->storeHsp();
	}
	elsif ($name eq 'BlastOutput_param')
	{
		$self->{options}->{program} = $self->{BlastOutput_program};
		$self->{options}->{filter} = $self->{Parameters_filter};
		$self->{options}->{dbs} = $self->{BlastOutput_db};
		$self->{options}->{matrix} = $self->{Parameters_matrix};
		$self->{options}->{expect} = $self->{Parameters_expect};
#		$self->{options}->{wordsize} = $self->{Parameters_};
		
		$self->{param_section} = 0;
	}
	
	$self->{BlastElement} = undef;
}

sub characters
{
	my ($self, $el) = @_;
	
	$self->{Data} .= $el->{Data};
}

1;

