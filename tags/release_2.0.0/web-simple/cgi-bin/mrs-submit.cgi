#!/usr/bin/perl

use strict;
use MRSCGI;
use CGI::Cookie;
use CGI::Carp qw(fatalsToBrowser);
use Data::UUID;
use Data::Dumper;
use POSIX qw(strftime);

my $q = new MRSCGI(script=>'mrs-submit.cgi');
my $ug = new Data::UUID;

#my $SGE_ROOT = '/usr/data/gridware/sge/';
#my $qsub = "$SGE_ROOT/bin/lx24-amd64/qsub";

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

my $work_dir = $mrs_tmp_dir;
my $base_url = $q->base_url;

&main();
exit;

sub submitJobs()
{
	my $date = strftime("%Y-%m-%d %H:%M:%S", localtime());
	
	my $desc = "Query submitted at $date ";
	$desc .= $q->param('file') if defined $q->param('file');
	
#	my $program = $q->param('program') or die "No program specified";
	my $program = 'blastp';
	my @dbs = split(m/,\s*/, $q->param('dbs')) or die 'No databanks specified';

	my $options = ' ';

	if (defined $q->param('filter') and $q->param('filter') eq 'on')
	{
		$options .= ' -F T';
	}
	else
	{
		$options .= ' -F F';
	}
	
#	$options .= ' -v ' . $q->param('limit_nr') . ' -b ' . $q->param('limit_nr')
#		if (defined $q->param('limit') and $q->param('limit') eq 'on' and $q->param('limit_nr') != 0);
#
	$options .= ' -W ' . $q->param('wordsize')
		if (defined $q->param('wordsize') and $q->param('wordsize') ne 'Default');
	
	$options .= ' -M ' . $q->param('matrix')
		if (defined $q->param('matrix') and $q->param('program') ne 'blastn');

	$options .= ' -e ' . $q->param('expect')
		if (defined $q->param('expect') and $q->param('expect') != 0);

	if (defined $q->param('gapped') and $q->param('gapped') eq 'on')
	{
		$options .= ' -g T ';
		
		$options .= ' -G ' . $q->param('gapopen')
			if (defined $q->param('gapopen') and $q->param('gapopen') ne 'Default');

		$options .= ' -E ' . $q->param('gapextend')
			if (defined $q->param('gapextend') and $q->param('gapextend') ne 'Default');
	}
	else
	{
		$options .= ' -g F ';
	}

	$options .= ' -l "' . $q->param('mrs_query') . '" '
		if (defined $q->param('mrs_query') and length $q->param('mrs_query'));

	$options .= ' -d "' . join(' ', @dbs) . '" ';
	
#	$options .= ' -m 7 ';
#	$options .= ' -a 2 ';

	# and now fetch the sequences in the fasta formatted query.
	
	my ($seq, $id);
	
	my $query = $q->param('query');
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
	
	die "No valid query was submitted" if scalar @queries == 0;

	my $nr = 1;
	++$nr while -f sprintf("query_%4.4d.csh", $nr);
	
	my $stored_options;
	
	foreach my $f (@queries)
	{
		my $query_name = sprintf("query_%4.4d", $nr++);
		
		open FASTA, ">$query_name.fa";
		print FASTA ">$f->{id} $f->{desc}\n$f->{seq}\n";
		close FASTA;
		
		open SCRIPT, ">$query_name.csh";
		print SCRIPT "#!/bin/tcsh\n";
		print SCRIPT "#\$ -N blast_$query_name\n";
		print SCRIPT "cd $work_dir\n";
		print SCRIPT "( /usr/bin/env MRS_DATA_DIR=$mrs_data /opt/local/bin/mrs_blast -p $program $options -i $query_name.fa -o $query_name.out ) >& $query_name.err\n";
		close SCRIPT;
		
		chmod 0744, "$query_name.csh";
		
		open PARAMS, ">$query_name.params-mrs";
		$q->save(\*PARAMS);
		close PARAMS;
		
#		my $submission_id = `. $SGE_ROOT/default/common/settings.sh; $qsub -cwd /$work_dir/$query_name.csh`;
		system("batch -f /$work_dir/$query_name.csh");
#		
#		print $q->p("job id: $submission_id");
	}
}

sub main()
{
	eval
	{
		my $session_id = $q->param('session_id') or die 'No session id specified';
	
		$work_dir .= $session_id;
		if (not -d $work_dir)
		{
			mkdir($work_dir) or die "Could not create working directory: $!";
		}
		
		chdir($work_dir) or die "Could not change directory to $work_dir: $!";
		
		&submitJobs();
		
		print $q->redirect("$base_url/cgi-bin/mrs-result.cgi");
	};
	
	if ($@)
	{
		print
			$q->header(-charset=>'utf-8'),
			$q->start_html({
				-title=>'Blast submission page',
	#			-script=>{'src'=>"$base_url/mrs.js", 'src'=>"$base_url/blast.js"},
				-style=>{'src'=>"$base_url/mrs_style.css"}
			}),
			$q->p($@),
			$q->p("An error occurred trying to submit your job: $!"),
			$q->p("Did you enter a sequence in FastA format?"),
			$q->end_html;
	}
}
