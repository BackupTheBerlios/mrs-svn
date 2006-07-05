#!/usr/bin/perl

use strict;
use MRSCGI;
use CGI::Cookie;
#use CGI::Pretty;
use CGI::Carp qw(fatalsToBrowser);
use Data::UUID;
use Data::Dumper;
use Format;
use MRS;

my $q = new MRSCGI(script=>'mrs-clustalw.cgi');
my $ug = new Data::UUID;

my %cookies = fetch CGI::Cookie;

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

sub colorAlignment
{
	my ($q, $url, $id, $a) = @_;

	my $line = $q->a({-href=>$url.$id}, $id);
				
	$line .= substr("                  ", 0, 17 - length($id));
	
	$a =~ s|([DE]+)|<span class="c1">$1</span>|g;
	$a =~ s|([KR]+)|<span class="c2">$1</span>|g;
	$a =~ s|([H]+)|<span class="c3">$1</span>|g;
	$a =~ s|([NQ]+)|<span class="c4">$1</span>|g;
	$a =~ s|([CM]+)|<span class="c5">$1</span>|g;
	$a =~ s|([GP]+)|<span class="c6">$1</span>|g;
	$a =~ s|([AVLIWF]+)|<span class="c7">$1</span>|g;
	$a =~ s|([YST]+)|<span class="c8">$1</span>|g;
	
	return "$line$a\n";
}

sub main()
{
	my $session_id = $q->param('session_id');
	$session_id = $ug->create_str() unless defined $session_id;
	
	my $session_cookie	= getCookie('session_id', $session_id, 1);
	$session_id = $session_cookie->value;

	$work_dir .= $session_id;
	if (not -d $work_dir)
	{
		mkdir($work_dir) or die "Could not create working directory: $!";
	}
	
	chdir($work_dir) or die "Could not change directory to $work_dir: $!";
	
	my $query;

	print
		$q->header(
			-cookie=>[$session_cookie],
			-charset=>'utf-8',
			-expires=>'now'
		),
		$q->start_html({
			-title=>'ClustalW',
			-script=>[
				{ 'src'=>$q->base_url . "/mrs.js"},
				{ 'src'=>$q->base_url . "/blast.js"},
				{ 'src'=>$q->base_url . "/result.js"}
			],
			-style=>{'src'=>$q->base_url . "/mrs_style.css"}
		});

	if ($q->param('query') or $q->param('file'))
	{
		print
			$q->menu(),
			'<div id="main">',
			$q->hidden('session_id', $session_id);
		
		$query = $q->param('query') unless defined $query;
		if (not defined $query or length($query) == 0)
		{
			if (length($q->param('file')))
			{
				my $filename = $q->param('file');
				
				local($/) = undef;
				$query = <$filename>;
			}
		}
	
		$query =~ s/\015\012/\012/;	# replace CRLF with LF characters
		$query =~ tr/\015/\012/;	# strip out the CR characters
		
		my @queries;
		my @lines = split(m/\n/, $query);
		
		while (my $line = shift @lines)
		{
			next unless $line =~ /^>(\S+)\s*(.+)?/;
			
			my %f;
			$f{id} = $1;
			$f{desc} = $2;
			$f{seq} = "";
			
			while (scalar @lines and substr($lines[0], 0, 1) ne '>')
			{
				$line = shift @lines;
				$line =~ s/[^a-zA-Z]+//g;
				$f{seq} .= $line;
			}
			
			$f{seq} =~ s/(.{72})/$1\n/g;
			push @queries, \%f;
		}
		
		die "Not enough sequences submitted to do a multiple alignment" if scalar @queries < 1;
		
		eval {
			open OUT, ">$$.fa";
			foreach my $s (@queries) {
				print OUT ">$s->{id} $s->{desc}\n$s->{seq}\n";
			}
			close OUT;
			
			my $diagnostics = "";
			my $alignment = "";
			
			open IN, "/opt/local/bin/clustalw -INFILE=$$.fa -TYPE=PROTEIN -OUTPUT=GDE -CASE=UPPER |";
			while (my $line = <IN>) {
				$diagnostics .= $line;
			}
			close IN;

			my (@parts, $acc, $part);

			open IN, "$$.gde";
			while (my $line = <IN>) {
				
				chomp($line);
				
				if ($line =~ /^%(\S+)/) {
					$acc = $1;
					undef $part;
				}
				else {
					if (not defined $part) {
						my %p = (
							acc => $acc,
							line => ""
						);
						$part = \%p;
						push @parts, $part;
					}
						
					$part->{line} .= $line;
				}
			}
			close IN;
			
			my $db = $q->param('db');
			$db = 'uniprot' unless defined $db;
			
			# first the unwrapped alignment
			
			foreach $part (@parts) {
				$alignment .= colorAlignment($q, $q->base_url."/cgi-bin/mrs.cgi?db=$db&id=", $part->{acc}, $part->{line});
			}
			
			$alignment = $q->div({id=>'unwrapped'}, $q->pre($alignment));
			
			# and now the wrapped alignment if lenght($part->{line}) > 60
			
			my $wrapped = "";
			
			while (length($part->{line}) > 0)
			{
				my $a = "";
				
				foreach $part (@parts) {
					my $seq = substr($part->{line}, 0, 60);
					last if (length($seq) == 0);
					
					$part->{line} = substr($part->{line}, 60);
					$a .= colorAlignment($q, $q->base_url."/cgi-bin/mrs.cgi?db=$db&id=", $part->{acc}, $seq);
				}
				
				$wrapped .= "$a\n";
			}

			$alignment .= $q->div({id=>'wrapped', style=>'display: none;'}, $q->pre($wrapped));
			
			print
				$q->div({-class=>'box'},
					$q->ul(
						$q->li(
							$q->span(
							{
								-class => 'switch',
								-onClick => 'showBox("jp", "diagnostic output")',
								-id => 'jp'
							},
							'Show diagnostic output'),
							$q->span(
							{
								-id=> 'clustal_show_hide_button',
								-class=>'switch',
								-onClick=>'toggleClustalColors();',
								-style=>'margin-left:100px'
							}, 'hide colors'),
							$q->span(
							{
								-id=> 'clustal_wrap_button',
								-class=>'switch',
								-onClick=>'toggleClustalWrap();',
								-style=>'margin-left:100px'
							}, 'show wrapped')
						),
						$q->li(
							$q->div({-id => 'jp_text', -style => 'display:none' },
								$q->pre($diagnostics)
							)
						)
					)
				),
				$q->div({-class=>'clustal', -id=>'clustal'}, $alignment);
		};
		
		return;
	}

	if ($q->param('db') and $q->param('id'))
	{
		my $db = $q->param('db');
		my $d = new MRS::MDatabank($db) or die "Could not open databank $db";
		$query = "";
		
		foreach my $id ($q->param('id')) {

			eval {
				my ($seq, $ix);
				$ix = 0;
				while ($seq = $d->Sequence($id, $ix++)) {
					$seq =~ s/(.{60})/$1\n/g;
					$query .= ">$id sequence nr $ix\n$seq\n";
				}
			};
			
			if ($@) {
				$query .= Format::fasta(&Filter($db), $q, $d->Get($id));
			}

#			$query .= Format::fasta(&Filter($db), $q, $d->Get($id));
		}

		$q->param(-name => 'query', -value => $query);
	}
	
	print
		$q->start_multipart_form(-name=>'f', -method=>'POST', -action => $q->base_url . "/cgi-bin/mrs-clustalw.cgi", -onSubmit => 'return doSubmit();'),

		$q->menu(),

		$q->div({-id => 'main'},
			$q->hidden('session_id', $session_id),

			$q->div({-id=>'s', -class=>'blackbox', -style=>'height:25px'},
				$q->div({-style=>'position:absolute; right: 10px;'},
					$q->submit(-name=>'clustalw', -value=>"Run ClustalW", -class=>'submit'),
				)
			),

			$q->div({-class => 'tabs'},
				$q->ul(
					$q->li({-id=>'prot_d', -class=>'selected'}, "Protein Sequences"),
				)
			),
			$q->div({-class=>'box'},
				$q->ul(
					$q->li(
						$q->span({-id=>'label1'}, "Enter several protein sequences in <em>FastA</em> format"),
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
		),
		$q->end_form,
		$q->end_html;
}
