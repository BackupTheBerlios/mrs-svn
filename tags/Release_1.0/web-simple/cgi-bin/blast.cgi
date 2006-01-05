#!/usr/pkg/bin/perl -w

use strict;
use MRSCGI;
use CGI::Cookie;
#use CGI::Pretty;
use CGI::Carp qw(fatalsToBrowser);
use Data::UUID;
use Data::Dumper;
use MRSCommon;

my $q = new MRSCGI(script=>'blast.cgi');
my $ug = new Data::UUID;

my %cookies = fetch CGI::Cookie;

my $blastdir = "/usr/data/blast/";
my $rawdir = "/usr/data/raw/";
my $work_dir = "/usr/tmp/blast/";

my $about =<<END;
<h2>About</h2>
<p><em>Abstract:</em> The biological data explosion of the 'omics' era requires fast
access to many data types in rapidly growing data banks. The MRS software provides
the tools to rapidly and reliably download, store, index, and query flat-file
databanks. Data stored and indexed by MRS takes considerably less space on disk
than the raw data, despite that these raw data are included. The MRS index
information is part of the stored data. Therefore, public and private data can be
combined by simple concatenation and thus without computational overheads. The
MRS software is freely available at
<a href='http://mrs.cmbi.ru.nl/download/'>http://mrs.cmbi.ru.nl/download/</a>
Users can choose to index their own data banks, or to obtain the weekly updated
MRS data files from this web address.</p>

<p>MRS was designed and implemented by Maarten Hekkelman at the CMBI. When using
this server or the software, please refer to:</p>

<p><cite>MRS: A fast and compact retrieval system for biological data. Hekkelman M.L.,
Vriend G., NAR submitted.</cite></p>

<p>If you have suggestions for improvement, please mail to
<a href="mailto:m.hekkelman\@cmbi.ru.nl?subject=MRS feedback">M.L. Hekkelman</a>.
END

my $db;
my @fields;
my %fldLabels;
my $exact = 0; # make this an option

our @dbs;
our $mrs_data;

my $settings = 'mrs.conf';
unless (my $return = do $settings)
{
	warn "couldn't parse $settings: $@" if $@;
	warn "couldn't do $settings: $!" unless defined $return;
	warn "couldn't run $settings" unless $return;
}

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

# make sure the plugin knows where to find the mrs files

$ENV{MRS_DATA_DIR} = $mrs_data;

my @formats = (
	'summary',
	'fasta',
	'entry'
);

my %formatLabels = (
	'fasta' => "FastA",
	'summary' => "Summary",
	'entry' => "Entry"
);

my @saveTo = (
	'text/plain',
	'text/x-mrs-plain'
);

my %saveToLabels = (
	'text/plain' => "to window",
	'text/x-mrs-plain' => "to file"
);

my @prot_dbs = 
[
	{	code=>'sptr', name=>"UniProt (SwissProt/TrEMBL (non redundant))",
		nodes=>[
			{ code=>'sprot', name=>"SwissProt" },
			{ code=>'trembl', name=>'TrEMBL' }
		]
	},
	{	code=>'refseqp', name=>'RefSeq' },
	{	code=>'remtrembl', name=>'RemTrEMBL' },
	{	code=>'pir', name=>'PIR' },
	{	code=>'gpcrdb', name=>'GPCRDB' },
	{	code=>'exprot', name=>'Exprot' },
	{	code=>'nucleardb', name=>'NuclearDB' },
	{	code=>'neurospora', name=>'Neurospora' },
];

my @nucl_dbs =
[
	{	code=>'embl', name=>"EMBL Nucleotide Databank",
		nodes=>[
			{ code=>'embl_main', name=>"EMBL Main Divisions",
				nodes=>[
					{ code=>'em_hum', name=>"EMBL Human Sequences" },
					{ code=>'em_inv', name=>"EMBL Invertebrate Sequences" },
					{ code=>'em_mam', name=>"EMBL Other Mammal Sequences" },
					{ code=>'em_mus', name=>"EMBL Mus musculus Sequences" },
					{ code=>'em_org', name=>"EMBL Organelle Sequences" },
					{ code=>'em_phg', name=>"EMBL Bacteriophage Sequences" },
					{ code=>'em_fun', name=>"EMBL Fungi Sequences" },
					{ code=>'em_pln', name=>"EMBL Plant Sequences" },
					{ code=>'em_pro', name=>"EMBL Prokaryote Sequences" },
					{ code=>'em_rod', name=>"EMBL Rodent Sequences" },
					{ code=>'em_syn', name=>"EMBL Synthetic Sequences" },
					{ code=>'em_unc', name=>"EMBL Unclassified Sequences" },
					{ code=>'em_vrl', name=>"EMBL Viral Sequences" },
					{ code=>'em_vrt', name=>"EMBL Other Vertebrate Sequences" },
				],
			},
			{ code=>'embl_est', name=>"EMBL EST Divisions",
				nodes=>[
					{ code=>'em_est_hum', name=>"EMBL Human EST's" },
					{ code=>'em_est_inv', name=>"EMBL Invertebrate EST's" },
					{ code=>'em_est_mam', name=>"EMBL Other Mammal EST's" },
					{ code=>'em_est_mus', name=>"EMBL Mus musculus EST's" },
					{ code=>'em_est_org', name=>"EMBL Organelle EST's" },
					{ code=>'em_est_phg', name=>"EMBL Bacteriophage EST's" },
					{ code=>'em_est_fun', name=>"EMBL Fungi EST's" },
					{ code=>'em_est_pln', name=>"EMBL Plant EST's" },
					{ code=>'em_est_pro', name=>"EMBL Prokaryote EST's" },
					{ code=>'em_est_rod', name=>"EMBL Rodent EST's" },
					{ code=>'em_est_syn', name=>"EMBL Synthetic EST's" },
					{ code=>'em_est_unc', name=>"EMBL Unclassified EST's" },
					{ code=>'em_est_vrl', name=>"EMBL Viral EST's" },
					{ code=>'em_est_vrt', name=>"EMBL Other Vertebrate EST's" },
				],
			},

			{ code=>'em_gss', name=>"EMBL GSS Section" },
			{ code=>'em_sts', name=>"EMBL STS Section" },
			{ code=>'em_htc', name=>"EMBL HTC Section" },
			{ code=>'em_pat', name=>"EMBL Patent Section" },
			{ code=>'em_wgs', name=>"EMBL WGS Section" },
		]
	},
	{	code=>'unigene', name=>"Unigene",
		nodes=>[
			# will be filled in below by script
		]
	},
	{	code=>'refseqn', name=>'RefSeq' },
	{	code=>'kabatn', name=>'Kabat' },
	{	code=>'vector', name=>'Vector',
		nodes=>[
			{ code=>'vector_cloned', name=>"Vector-Cloned" },
			{ code=>'vector_complete', name=>"Vector-Complete" },
			{ code=>'vector_noncomplete', name=>"Vector-Non Complete" },
		]
	},
	{	code=>'pdb_nucl', name=>'PDB' },
	{	code=>'neurospora', name=>'Neurospora' },
];

&updateUnigene(@nucl_dbs);
&main();
exit;

# add all available unigene nodes, piece of cake!

sub updateUnigene
{
	my ($tree) = @_;
	
	foreach my $node (@{$tree})
	{
		next unless $node->{name} eq 'Unigene';
		
		open UNI_LIST, "<unigene.list";
		my %unigenes = map { (split(m/\t/, $_))[0] => (split(m/[\t\n]/, $_))[1] } <UNI_LIST>;
		close UNI_LIST;
		
		opendir DIR, "$rawdir/unigene/";
		my @unigene_info_files = sort grep { -f "$rawdir/unigene/$_" and m/\.info$/ } readdir DIR;
		closedir DIR;

		my $new = 0;

		foreach my $unigene (@unigene_info_files)
		{
			$unigene =~ s/\.info$//;
		
			if (not defined $unigenes{"uni_$unigene"})
			{
				$new = 1;
				
				open INFO, "<$rawdir/unigene/$unigene.info";
				my $first_line = <INFO>;
				close INFO;
				
				chomp($first_line);
				
				if ($first_line =~ /^UniGene Build .\d+\s*(.+)/)
				{
					$unigenes{"uni_$unigene"} = "Unigene-$1";
				}
			}
		}
		
		if ($new)
		{
			open UNI_LIST, ">unigene.list";
			foreach my $k (sort keys %unigenes)
			{
				print UNI_LIST "$k\t$unigenes{$k}\n";
			}
			close UNI_LIST;
		}
		
		foreach my $k (sort keys %unigenes)
		{
			my %new_unigene = (
				code => $k,
				name => $unigenes{$k}
			);
			
			push @{$node->{nodes}}, \%new_unigene;
		}
	}
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

sub getSelected
{
	my ($a, $b) = @_;

	my $result = 'unselected';
	$result = 'selected' if ($a eq $b);
	
	return $result;
}

sub checkForAvailability
{
	my ($tree, $kind) = @_;

	my (@result, $al_ext, $sq_ext);
	
	if ($kind eq 'protein')
	{
		$al_ext = 'pal';
		$sq_ext = 'psq';
	}
	else
	{
		$al_ext = 'nal';
		$sq_ext = 'nsq';
	}

	foreach my $node (@{$tree})
	{
		if (defined $node->{nodes})
		{
			my $subtree = &checkForAvailability($node->{nodes}, $kind);
			if (scalar @{$subtree})
			{
				$node->{nodes} = $subtree;
				push @result, $node;
			}
		}
		else
		{
			my $code = $node->{code};
			
			push @result, $node if -f "$blastdir/$code.$al_ext" or -f "$blastdir/$code.$sq_ext";
		}
	}

	return \@result;
}

sub createNodes
{
	my ($data, $level, $prefix, $parent, $kind) = @_;
	
	my $n = scalar @{$data};
	my ($html, $script);
	
	foreach my $e (@{$data})
	{
		my $line = $prefix;
		my $hasNext = (--$n <= 0);

		my $label = $e->{name};
		my $code = $e->{code};
		
		if (defined $e->{nodes})
		{
			if ($hasNext)
			{
				$line .= $q->img({-src=>$q->base_url . "/img/plusbottom.gif", -onClick=>"t_$code.toggle();"});
			}
			else {
				$line .= $q->img({-src=>$q->base_url . "/img/plus.gif", -onClick=>"t_$code.toggle();"});
			}
		}
		else
		{
			if ($hasNext)
			{
				$line .= $q->img({-src=>$q->base_url . "/img/joinbottom.gif"});
			}
			else {
				$line .= $q->img({-src=>$q->base_url . "/img/join.gif"});
			}
		}
		
		my %options;
		$options{'-style'} = 'display:none' if ($level > 0);
		$options{'-id'} = "t_$code";
		
		my $cb_options = {
			-onClick =>	"t_$code.toggleSelect();",
			-name =>	$label,
			-value =>	$code,
			-id =>		"t_${code}_cb"
		};
		
		my $cookie = getCookie("t_${code}_cb");
		$cb_options->{-checked} = 'checked'
			if ($cookie and $cookie->value and ($cookie->value == 1 or $cookie->value eq 'true'));
		
		$html .= $q->li(\%options, $line, $q->checkbox($cb_options));
		$script .= "var t_$code = new tNode(\"$code\", $parent, \"$kind\");\n";
		
		if (defined $e->{nodes})
		{
			my ($h, $s);
			
			if ($hasNext)
			{
				($h, $s) = createNodes($e->{nodes}, $level + 1,
					$prefix . $q->img({-src=>$q->base_url . "/img/empty.gif"}), "t_$code", $kind);
			}
			else
			{
				($h, $s) = createNodes($e->{nodes}, $level + 1,
					$prefix . $q->img({-src=>$q->base_url . "/img/line.gif"}), "t_$code", $kind);
			}
			
			$html .= $h;
			$script .= $s;
		}
	}
	
	return ($html, $script);
}

sub createTree
{
	my ($kind) = @_;
	my ($html, $script);

	my $data;
	if ($kind eq "protein")
	{
		@prot_dbs = &checkForAvailability(@prot_dbs, $kind);

		($html, $script) = &createNodes(@prot_dbs, 0, "", "null", $kind);
	}
	else
	{
		@nucl_dbs = &checkForAvailability(@nucl_dbs, $kind);

		($html, $script) = &createNodes(@nucl_dbs, 0, "", "null", $kind);
	}
	
	return $q->ul({-class => 'tree'}, $html) . "<script language=\"JavaScript\">\n" . $script . "</script>\n";
}

sub main()
{
	my $onLoad = 'initBlastForm();';

	my $input_cookie	= getCookie('input', 'protein');		my $input = $input_cookie->value;
	my $db_cookie		= getCookie('db', 'protein');			my $db = $db_cookie->value;

	my $session_id = $q->param('session_id');
	$session_id = $ug->create_str() unless defined $session_id;
	
	my $session_cookie	= getCookie('session_id', $session_id, 1);
	$session_id = $session_cookie->value;

	if ($q->param('restore'))
	{
		$work_dir .= $session_id;
		
		chdir($work_dir) or die "Could not change directory to $work_dir: $!";

		my $query_name = $q->param('restore');
		
		if (-f "$query_name.params-ncbi")
		{
			my $base_url = $q->base_url;
			
			open PARAMS, "<$query_name.params-ncbi";
			$q->init(\*PARAMS);
			close PARAMS;

			$q->base_url($base_url);
			
			$q->delete('file');
			
			open INPUT, "<$query_name.fa";
			local($/) = undef;
			my $query = <INPUT>;
			close INPUT;

			$q->param(-name => 'query', -value => $query);
		}
		
		$input = $q->param('input');	$input_cookie->value($input);
		$db = $q->param('db');			$db_cookie->value($db);
		
		$q->delete('restore');
		$q->delete('session_id');
#		$onLoad = undef;
	}

	my $display_tree_prot = undef;
	my $display_tree_nucl = 'display:none';
	my $show_via_protein = 'display:none';
	
	my $matrix_style = undef;
	my @word_sizes = [ 'Default', '2', '3' ];
	if ($input eq 'nucleotide' and $db eq 'nucleotide')
	{
		$matrix_style = 'display:none';
		$show_via_protein = undef;
		@word_sizes = [ 'Default', '7', '8', '9', '10', '11', '12', '13', '14', '15', '16', '17', '18' ];
	}
	
	($display_tree_prot, $display_tree_nucl) = ($display_tree_nucl, $display_tree_prot)
		if ($db eq 'nucleotide');

	print
		$q->header(
			-cookie=>[$input_cookie, $db_cookie, $session_cookie],
			-charset=>'utf-8',
			-expires=>'now'
		),
		$q->start_html({
			-title=>'Blast',
			-script=>{'src'=>$q->base_url . "/mrs.js", 'src'=>$q->base_url . "/blast.js"},
			-style=>{'src'=>$q->base_url . "/mrs_style.css"},
			-onLoad=>$onLoad}),
			$q->start_multipart_form(-name=>'f', -method=>'POST', -action => $q->base_url . "/cgi-bin/submit.cgi", -onSubmit => 'return doSubmit();'
		),

		$q->menu($q->li($q->submit(-name => 'run', -value => 'Run Blast', -class => 'blast_submit'))),

		$q->div({-id => 'main'},
			$q->hidden('program', ''),
			$q->hidden('dbs', ''),
			$q->hidden('input', $input),
			$q->hidden('db', $db),
			$q->hidden('session_id', $session_id),
			$q->hidden('via_protein', '0'),
			$q->div({-class => 'tabs'},
				$q->ul(
					$q->li({-id=>'prot', -class=>&getSelected($input, 'protein'),
						-onClick=>"selectInput('protein');"}, "Protein Query"),
					$q->li({-id=>'nucl', -class=>&getSelected($input, 'nucleotide'),
						-onClick=>"selectInput('nucleotide');"}, "Nucleotide Query")
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li(
						$q->span({-id=>'label1'}, "Enter one or more $input sequences in <em>FastA</em> format"),
						$q->br(),
						$q->textarea('query','',6,80)
					),
					$q->li(
						"Or enter filename:",
						$q->filefield(-name=>'file',
			        		-size=>50,
			        		-maxlength=>80
			            )
					)
				),
			),
			$q->br(),
			$q->div({-class=>'tabs'},
				$q->ul(
					$q->li({-id=>'prot_d', -class=>&getSelected($db, 'protein'),
						-onClick=>"selectDbs('protein');"}, "Protein Databanks"),
					$q->li({-id=>'nucl_d', -class=>&getSelected($db, 'nucleotide'),
						-onClick=>"selectDbs('nucleotide');"}, "Nucleotide Databanks")
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li(
						$q->span({-id=>'prot_dbs', -style=>$display_tree_prot, class=>'tree'},
							&createTree("protein")
						),
						$q->span({-id=>'nucl_dbs', -style=>$display_tree_nucl, class=>'tree'},
							&createTree("nucleotide")
						)
					)
				)
			),
			$q->br(),
			$q->div({-class=>'tabs'},
				$q->ul(
					$q->li({-id=>'blast', -class=>'selected',
						-onClick=>"selectProgram('blast');"}, "Blast Options"),
	#					$q->li({-id=>'nucl', -class=>'unselected',
	#						-onClick=>"selectProgram('fasta');"}, "Fasta")
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li({-id=>'bo1'},
						$q->checkbox(
							-label =>	'Limit output to',
							-name =>	'limit',
							-value =>	'on',
							-checked =>	'checked'),
						$q->textfield({-name=>'limit_nr', -size=>4, -default=>"100"}),
						"best hits per sequence"
					),
					$q->li({-id=>'bo_filter'},
						$q->checkbox(-name=>'filter',
							-checked=>'checked',
							-value=>'on',
							-label=>"Filter query sequence (low complexity)"
						)
					),
					$q->li({-id=>'bo_via_protein', -style=>$show_via_protein},
						$q->checkbox(
							-name=>'via_protein',
							-id=>'via_protein',
							-value=>'on',
							-label=>"Search using protein translation (i.e. use TBlastX)",
							-onClick=>'updateOptions(); return true;'
						)
					),
					$q->li({-id=>'bo_matrix', -style=>$matrix_style},
						"Scoring matrix",
						$q->popup_menu(
							-name=>'matrix',
							-values=>['BLOSUM45', 'BLOSUM62', 'BLOSUM80', 'PAM30', 'PAM70'],
							-default=>'BLOSUM62'
						)
					),
					$q->li({-id=>'bo_wordsize'},
						"Wordsize",
						$q->popup_menu(-name=>'wordsize', -values=>@word_sizes, -default=>'Default')
					),
					$q->li({-id=>'bo_evalue'},
						"E-value cutoff",
						$q->popup_menu(
							-name=>'expect',
							-values=>['0.0001', '0.001', '0.01', '0.1', '1', '10', '100', '1000'],
							-default=>'10'
						)
					),
					$q->li({-id=>'bo_gapped'},
						$q->checkbox(
							-id=>'bo_gapped_cb',
							-name=>'gapped',
							-checked=>'checked',
							-value=>'on',
							-label=>"Perform gapped alignment",
							-onClick => 'selectGapped();'
						)
					),
					$q->li({-id=>'bo_gapopen'},
						"Cost to open a gap",
						$q->popup_menu(
							-name=>'gapopen',
							-values=>['Default', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14'],
							-default=>'Default'
						)
					),
					$q->li({-id=>'bo_gapextend'},
						"Cost to extend a gap",
						$q->popup_menu(
							-name=>'gapextend',
							-values=>['Default', '1', '2', '3', '4'],
							-default=>'Default'
						)
					),
				),
#					$q->ul({-id=>'fasta_options', -style=>'display:none'},
#						$q->li(
#						),
#						$q->li(
#						)
#					),
			),
			$q->end_form
		);

	print
		$q->end_form,
		$q->end_html;
}
