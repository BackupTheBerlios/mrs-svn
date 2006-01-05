#!/usr/pkg/bin/perl -w

use strict;
use MRSCGI;
use CGI::Cookie;
#use CGI::Pretty;
use CGI::Carp qw(fatalsToBrowser);
use Data::UUID;
use Data::Dumper;
use MRSCommon;
use Format;
use MRS;

my $q = new MRSCGI(script=>'mrs-blast.cgi');
my $ug = new Data::UUID;

my %cookies = fetch CGI::Cookie;

my $db;
my @fields;
my %fldLabels;
my $exact = 0; # make this an option

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

my @dbIds = map { $_->{id} } @dbs;
my %dbLabels = map { $_->{id} => $_->{name} } @dbs;
my %dbFilters = map { $_->{id} => $_->{filter} } @dbs;
my %dbInfo = map { $_->{id} => $_ } @dbs;

my $work_dir = $mrs_tmp_dir;

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

&main();
exit;

sub Filter
{
	my $db = shift;
	
	my $result = $dbFilters{$db};
	if (not defined $result)
	{
		foreach my $db (split(/\|/, $db))
		{
			$result = $dbFilters{$db};
			last if defined $result;
		}
	}
	
	return $result;
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

sub getDbPopup($$)
{
	my ($default, $script) = @_;
	
	my @ids = @dbIds;
	
	my (@values, %labels);
	
	while (my $db = shift @ids)
	{
		if ($dbInfo{$db}->{blast})
		{
			push @values, $db;
			$labels{$db} = $dbInfo{$db}->{name};
		}
	}
	
	my $result = $q->popup_menu(
		-name=>"dbs",
		-values=>\@values,
		-default=>$default,
		-labels=>\%labels,
		-onChange=>$script
	);
	
	return $result;
}


sub main()
{
	my $session_id = $q->param('session_id');
	$session_id = $ug->create_str() unless defined $session_id;
	
	my $session_cookie	= getCookie('session_id', $session_id, 1);
	$session_id = $session_cookie->value;
	
	my ($input, $db);

	if ($q->param('restore'))
	{
		$work_dir .= $session_id;
		
		chdir($work_dir) or die "Could not change directory to $work_dir: $!";

		my $query_name = $q->param('restore');
		
		if (-f "$query_name.params-mrs")
		{
			my $base_url = $q->base_url;
			
			open PARAMS, "<$query_name.params-mrs";
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
		
#		$input = $q->param('input');	$input_cookie->value($input);
#		$db = $q->param('db');			$db_cookie->value($db);
		
		$q->delete('restore');
		$q->delete('session_id');
#		$onLoad = undef;
	}
	elsif ($q->param('db') and $q->param('id'))
	{
#		eval
#		{
			my $db = new MRS::MDatabank($q->param('db'));
			my $text = $db->Get($q->param('id'));
			
			my $query = &Format::fasta(&Filter($q->param('db')), $q, $text);
			
			$q->param(-name => 'query', -value => $query);
#		};
		
#		if ($@)
	}
	
	my $show_via_protein = "display:none";
	my $matrix_style = "";
	my @word_sizes = ( "2", "3", "4" );

	print
		$q->header(
#			-cookie=>[$input_cookie, $db_cookie, $session_cookie],
			-cookie=>[$session_cookie],
			-charset=>'utf-8',
			-expires=>'now'
		),
		$q->start_html({
			-title=>'Blast',
			-script=>{'src'=>$q->base_url . "/mrs.js", 'src'=>$q->base_url . "/blast.js"},
			-style=>{'src'=>$q->base_url . "/mrs_style.css"}
		}),

		$q->start_multipart_form(-name=>'f', -method=>'POST', -action => $q->base_url . "/cgi-bin/mrs-submit.cgi", -onSubmit => 'return doSubmit();'),

		$q->menu(),#$q->li($q->submit(-name => 'run', -value => 'Run Blast', -class => 'blast_submit'))),

		$q->div({-id => 'main'},
			$q->hidden('session_id', $session_id),
			$q->hidden('program', 'blastp'),

#			$q->div({-id=>'s', -class=>'blackbox'},
#				$q->reset(-class=>'submit', -name=>'Clear', -style=>'margin-left: 10px;'),
#				$q->submit(-name=>'clustalw', -value=>"Run Blast",
#					-class=>'submit', -style=>'position:absolute; right: 10px;'),
#			),
			$q->div({-id=>'s', -class=>'blackbox', -style=>'height:25px'},
				$q->div({-style=>'position:absolute; right: 10px;'},
#					$q->reset(-class=>'submit', -name=>'Clear', -style=>'margin-left: 10px;'),
					$q->submit(-name=>'clustalw', -value=>"Run Blast",
						-class=>'submit'),
				)
			),

			$q->div({-class => 'tabs'},
				$q->ul(
					$q->li({-id=>'prot_d', -class=>'selected'}, "Protein Query"),
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li(
						$q->span({-id=>'label1'}, "Enter one or more protein sequences in <em>FastA</em> format"),
						$q->br,
						$q->textarea('query','',6,80)
					),
					$q->li(
						$q->span("Or enter filename:"),
						$q->filefield(-name=>'file',
			        		-size=>50,
			        		-maxlength=>80
			            ),
			       	)
				)
			),
			$q->br(),
			$q->div({-class=>'tabs'},
				$q->ul(
					$q->li({-id=>'prot_d', -class=>'selected'}, "Protein Databanks"),
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li(
						$q->span({-id=>'label1'}, "Choose the databank to search"),
						&getDbPopup('sprot'),
					),
					$q->li(
						$q->span({-id=>'label1'}, "Optionally enter an MRS query to limit the search space"),
						$q->textfield(
							-name=>"mrs_query",
							-id=>"mrs_query",
							-class=>'tb',
							-size=>40,
							-maxlength=>256,
#							-onKeyPress=>'doKeyPress(event);',
							-onKeyUp=>'updateQueryCount();'
						),
						$q->div({-id=>"query_count", -style=>'display:none; color: red'}),
					)
				)
			),
			$q->br(),
			$q->div({-class=>'tabs'},
				$q->ul(
					$q->li({-id=>'opt_reg_tab', -class=>'selected', -onClick=>'selectParameters("reg");'}, "Blast Options"),
					$q->li({-id=>'opt_adv_tab', -class=>'unselected', -onClick=>'selectParameters("adv");'}, "Advanced Blast Options"),
				)
			),
			$q->div({-class=>'box'},

				$q->ul({-id=>'opt_reg_box'},
#					$q->li({-id=>'bo1'},
#						$q->checkbox(
#							-label =>	'Limit output to',
#							-name =>	'limit',
#							-value =>	'on',
#							-checked =>	'checked'),
#						$q->textfield({-name=>'limit_nr', -size=>4, -default=>"100"}),
#						"best hits per sequence"
#					),
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
					$q->li({-id=>'bo_evalue'},
						"E-value cutoff",
						$q->popup_menu(
							-name=>'expect',
							-values=>['0.0001', '0.001', '0.01', '0.1', '1', '10', '100', '1000'],
							-default=>'10'
						)
					),
				),
				$q->ul({-id=>'opt_adv_box', -style=>'display:none;'},
					$q->li({-id=>'bo_wordsize'},
						"Wordsize",
						$q->popup_menu(-name=>'wordsize', -values=>\@word_sizes, -default=>'3')
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

#				$q->ul(
#					$q->li(
#						$q->span({-id=>'label1'}, "This blast uses the following options which cannot be altered")
#					),
#					$q->li(
#						"Using low complexity filter (SEG)"
#					),
#					$q->li(
#						"BLOSUM62 Matrix"
#					),
#					$q->li(
#						"Gapped alignment"
#					),
#					$q->li(
#						"Gap open cost 11"
#					),
#					$q->li(
#						"Gap extend cost 1"
#					),
#					$q->li(
#						"E-value cutoff 10.0"
#					),
#					$q->li(
#						"Limit output to the first 500 hits"
#					),
#				),
			),
		);

	print
		$q->end_form,
		$q->end_html;
}
