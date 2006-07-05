#!/usr/bin/perl
#
# $Id: mrs.cgi,v 1.36 2005/10/27 10:59:11 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use MRSCGI;
use CGI::Cookie;
use CGI::Carp qw(fatalsToBrowser);
#use CGI::Pretty;
use MRS;
use File::stat;
use Data::Dumper;
use Format;

my $q = new MRSCGI(script => 'mrs.cgi');

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
Vriend G.<br/>
<a href="http://nar.oxfordjournals.org/cgi/content/full/33/suppl_2/W766?ijkey=1hM9Po54JADYz0b&keytype=ref">
Nucleic Acids Research 2005 33(Web Server issue):W766-W769; doi:10.1093/nar/gki422.</a></cite></p>

<p>The source code for MRS is now available from:
<a href="http://developer.berlios.de/projects/mrs/">http://developer.berlios.de/projects/mrs/</a></p>


<a href="http://developer.berlios.de" title="BerliOS Developer"> <img src="http://developer.berlios.de/bslogo.php?group_id=5642" width="124px" height="32px" border="0" alt="BerliOS Developer Logo"></a>

<p>If you have suggestions for improvement, please mail to
<a href="mailto:m.hekkelman\@cmbi.ru.nl?subject=MRS feedback">M.L. Hekkelman</a>.

<p>There's also a mailinglist for issues related with MRS, to subscribe go to
<a href="http://lists.berlios.de/mailman/listinfo/mrs-user">http://lists.berlios.de/mailman/listinfo/mrs-user</a>.


END

my $db;
my @fields;
my %fldLabels;
my $exact = 0; # make this an option

our @dbs;
our $mrs_data;
our $mrs_script;

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

my %index_types = (
	'text' => 'Text',
	'date' => 'Date',
	'valu' => 'Value',
	'nmbr' => 'Number'
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

sub printNumber
{
	my $n = shift;

	1 while $n =~ s/(\d)(\d\d\d)(?!\d)/$1,$2/;
	
	return $n;
}

sub printEntryHeader
{
	my ($db, $id) = @_;
	my $db_esc = $db;
	$db_esc =~ s/\+/%2B/g;
	
	my @btns;
	
	my $url = $q->url({-full=>1});
	my $base_url = $q->base_url;
	
	push @btns, $q->li([
		"View",
		$q->popup_menu(
			-name=>"format",
			-values=>\@formats,
			-default=>'summary',
			-labels=>\%formatLabels,
			-onChange=>"doReloadEntry('$url', '$db_esc', '$id');"
		),
		$q->button(
			-name=>'save',
			-value=>"Save",
			-class=>'submit',
			-onClick=>"doSave('$url', '$db_esc', '$id')"
		),
		$q->popup_menu(
			-name=>"save_to",
			-values=>\@saveTo,
			-labels=>\%saveToLabels
		),
		$q->button(
			-name=>'blast',
			-value=>"Blast",
			-class=>'submit',
			-onClick=>"window.location = '$base_url/cgi-bin/mrs-blast.cgi?db=$db_esc&id=$id';"
		),
	]);

	print $q->div({-class=>'nav'},
		$q->hidden('id2', $id),
		$q->ul(@btns)
	);
}

sub getDbPopup($$)
{
	my ($default, $script) = @_;
	
	my @ids = @dbIds;
	
	my (@values, %labels);
	
	while (my $db = shift @ids)
	{
		if ($dbInfo{$db}->{group})
		{
			my $group = $dbInfo{$db}->{group};
			my (@group_values, %group_labels);
			
			push @group_values, $db;
			$group_labels{$db} = $dbInfo{$db}->{name};
			
			while (scalar @ids and $dbInfo{$ids[0]}->{group} eq $group)
			{
				$db = shift @ids;
				push @group_values, $db;
				$group_labels{$db} = $dbInfo{$db}->{name};
			}
			
			push @values, $q->optgroup(
				-name => $group,
				-values => \@group_values,
				-labels => \%group_labels);
		}
		else
		{
			push @values, $db;
			$labels{$db} = $dbInfo{$db}->{name};
		}
	}
	
	my $result = $q->popup_menu(
		-name=>"db",
		-values=>\@values,
		-default=>$default,
		-labels=>\%labels,
		-onChange=>$script,
		-style=>'max-width:120px;'
	);
	
	return $result;
}

sub doDisplayEntry($$$$)
{
	my ($db, $format, $id, $d) = @_;
	my $filter = 1;
	
	my $text = $d->Get($id);

	&printEntryHeader($db, $id);
	
	if (defined $format and $format eq 'fasta')
	{
		eval {
			my ($seq, $ix);
			$ix = 0;
			while ($seq = $d->Sequence($id, $ix++)) {
				$seq =~ s/(.{60})/$1\n/g;
				print $q->pre(">$id sequence nr $ix\n$seq");
			}
		};
		
		if ($@) {
			print $q->pre(Format::fasta(&Filter($db), $q, $text));
		}
	}
	else
	{
		print Format::pre(&Filter($db), $q, $text, $id);
	}
}

sub quoteQuery($)
{
	my $query = shift;
	
	$query =~ s/'/\\'/g;
	$query =~ s/"/\\"/g;
	
	return "\"$query\"";
}

sub printListHeader
{
	my ($first, $count, $max, $query) = @_;
	
	my $last = $first + $count;
	my $prev = $first - $count;
	my $next = $first + $count;

		# the user doesn't like zero based...
	++$first;

	$last = $max if $last > $max;
	$prev = 0 if $prev < 0;

	my @btns;
	
	my $max_display = $max;
	$max_display = sprintf("≈%s,000", &printNumber(sprintf("%.0f", $max / 1000))) if (not $exact and $max >= 1000); #/
	
	push @btns, $q->li("Items $first-$last of $max_display");
#	push @btns, $q->li({-style=>'width:60px'}, "&nbsp;");

	if ($first <= 1) {
		push @btns, $q->li({-class=>'disabled'}, "Prev");
	}
	else {
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>"javaScript:doPage($prev," . &quoteQuery($query) . ")"}, "Prev"));	
	}
	
	if ($next >= $max) {
		push @btns, $q->li({-class=>'disabled'}, "Next");
	}
	else {
		push @btns, $q->li({-class=>'enabled'},
			$q->a({-href=>"javaScript:doPage($next," . &quoteQuery($query) . ")"}, "Next"));
	}

#	push @btns, $q->li({-style=>'width:60px'}, "&nbsp;");
	
	my $base_url = $q->base_url;
	
	push @btns, $q->li([
		"View",
		$q->popup_menu(
			-name=>"format",
			-values=>\@formats,
			-default=>'summary',
			-labels=>\%formatLabels,
			-onChange=>"doPage($first-1, " . &quoteQuery($query) . ");"
		),
		"Show",
		$q->textfield(
			-name=>"count",
			-size=>3,
			-maxlength=>4,
			-default=>'15',
			-onKeyPress=>'doKeyPress(event);'
		),
		$q->button(
			-id=>'save',
			-name=>'save',
			-value=>"Save",
			-class=>'submit',
			-onClick=>"doSave('".$q->url({-full=>1})."')"),
		$q->popup_menu(
			-name=>"save_to",
			-values=>\@saveTo,
			-labels=>\%saveToLabels
		),
		$q->button(
			-name=>'clustalw',
			-value=>"ClustalW",
			-class=>'submit',
			-onClick=>"doSave('$base_url/cgi-bin/mrs-clustalw.cgi');"
		),
	]);

	print $q->div({-class=>'nav'}, $q->ul(@btns));
}

sub doList
{
	my ($db, $format, $first, $count, $query, $wildcard, $d) = @_;
	
	my $i;
	
	eval
	{
		$i = $d->Find($query, $wildcard);
	};
	
	if ($@ or not defined $i)
	{
		print $q->p("No hits found");
	}
	elsif ($i->Count(0) == 1 and (not defined $format or $format eq 'summary'))
	{
		doDisplayEntry($db, 'entry', $i->Next, $d);
	}
	else
	{
		# mind the + 1, needed because we're counting 1 based here... sigh
		my $max = $i->Count($exact);
		$count = $max if $count > $max;
		my $n;
		
		my @rows;
		push @rows, $q->Tr(
			$q->th('nr'),
			$q->th('id'),
			$q->th('description'),
			$q->th('&nbsp;')
		);
	
		$i->Skip($first) if $first > 0;
		
		for (my $id = $i->Next; defined $id and ++$n <= $count; $id = $i->Next)
		{
			if (defined $format and $format eq 'fasta')
			{
				my $fasta = "";

				eval {
					my ($seq, $ix);
					$ix = 0;
					while ($seq = $d->Sequence($id, $ix++)) {
						$seq =~ s/(.{60})/$1\n/g;
						$fasta .= $q->pre(">$id sequence nr $ix\n$seq");
					}
				};
				
				if ($@) {
					$fasta = $q->pre(Format::fasta(&Filter($db), $q, $$d->Get($id)));
				}
				
				push @rows, $q->Tr($q->td({-colspan=>4}, $q->pre($fasta)));
			}
			elsif (defined $format and $format eq 'entry')
			{
				push @rows, $q->Tr(
					$q->td({-colspan=>4}, $q->pre($d->Get($id)))
				);
			}
			else
			{
				my $url = $q->url({-full=>1});
				my $db_esc = $db;
				$db_esc =~ s/\+/%2B/;
				
				$url .= "?db=$db_esc&id=$id&format=entry";
				$url =~ s/#/%23/;
				
				push @rows, $q->Tr(
					$q->td($q->b($first + $n)),
					$q->td($q->a({-href=>$url}, $id)),
					$q->td(Format::desc(&Filter($db), $q, $d->Get($id))),
					$q->td(
						$q->checkbox(
							-id=>$id,
							-name=>$id,
							-value=>'on',
							-label=>'',
							-onMouseDown=>'return doMouseDownOnCheckbox(event);',
							-onClick=>"return doClickSelect(\'$id\')"
						)
					)
				);
			}
		}
		
		if ($n < $count)
		{
			$max = $first + $n + 1;
			$count = $n + 1;
		}

		&printListHeader($first, $count, $max, $query);

		print $q->div({-class=>'list', -id=>'list'},
			$q->table({-cellspacing=>'0', -cellpadding=>'0'}, @rows));
	}

	eval {
		my @query_parts = split(m/\s+/, $query);
		my $url = $q->url({-absolute=>1});
		
		my (@suggestions, $andOrWarning);
		
		for (my $i = 0; $i < scalar(@query_parts); ++$i)
		{
			my $ix;
			my $term = $query_parts[$i];
			
			next if $term =~ /[<=>]/;
			
			if ($term =~ /(\w+):(.+)/)
			{
				$ix = $1;
				$term = $3;
			}
			
			if ($term eq 'and' or $term eq 'or' or $term eq 'not')
			{
				$andOrWarning = $q->p($q->strong("Note:"), "The words '<code>and</code>, '<code>or</code>' and '<code>not</code>'",
					" can be used as boolean operators. But then they have to be in upper case.");
			}
			
			my $alt_iter = $d->SuggestCorrection($term);
			
			if (defined $alt_iter)
			{
				my $alt;
				while (defined ($alt = $alt_iter->Next))
				{
					if ($alt ne $term and length($alt) > 0)
					{
						$alt = "$ix:$alt" if defined $ix;
						
						my @alt_query_parts = @query_parts;
						
						$alt_query_parts[$i] = $alt;
						
						my $alt_query = join(' ', @alt_query_parts);
						
						my ($iter, $count);
						if ($iter = $d->Find($alt_query, 0) and $count = $iter->Count(0))
						{
							my $db_esc = $db;
							$db_esc =~ s/\+/%2B/;
							
							push @suggestions, $q->li($q->a({-href=>"$url?db=$db_esc&query=$alt_query"}, $alt_query), " ($count)");
						}
					}
				}
			}
		}

		print $andOrWarning if defined $andOrWarning;

		if (scalar @suggestions)
		{
			print
				$q->p("Alternative spellings you might want to try:\n"),
				$q->ul(@suggestions);
		}
	}
}

sub doListAll()
{
	my ($query, $wildcard) = @_;

	$| = 1;
	
	print "<div class='list'><table cellspacing='0' cellpadding='0'>\n";
	print "<tr><th>Databank</th><th>Entries found</th></tr>\n";
	
	my ($db_cnt, $entry_cnt, $max_count) = ( 0, 0, 0 );
	
	foreach my $db (@dbIds)
	{
		next if $db eq 'all';

		my $d;
		eval { $d = new MRS::MDatabank($db); };
		next if $@;
		
		++$db_cnt;
		$entry_cnt += $d->Count;
		
		if (not defined $d)
		{
			print $q->p("Could not open databank $db");
			next;
		}

		eval
		{
			if (my $i = $d->Find($query, $wildcard))
			{
				my $url = $q->url({-full=>1, -query=>1});
				
				my $db_esc = $db;
				$db_esc =~ s/\+/%2B/;
				
				$url =~ s/all/$db_esc/;
				
				my $count = $i->Count($exact);
				$max_count = $count if $count > $max_count;
	
				my $count_display = $count;
				
				if (not $exact and $count >= 1000)
				{
					$count = sprintf("%.0f", $count/1000);	#/
					$count_display = sprintf("≈%s,000", &printNumber($count));
				}
				
				print "<tr><td><a href='$url'>$dbLabels{$db}</a></td>";
				print "<td style='text-align:right'>$count_display</td></tr>\n";
			}
		};
	}

	print "</div>";
	
	$entry_cnt = &printNumber($entry_cnt);
	print $q->p("Done! Searched $db_cnt databanks containing $entry_cnt records");
}

sub printOverview()
{
	my @rows;
	my $name;
	
	push @rows, $q->caption('Indexed databanks');
	push @rows, $q->Tr(
		$q->th('Name'),
		$q->th('ID'),
		$q->th('Entries'),
		$q->th({-style=>'width:12%'}, 'Version'),
		$q->th({-style=>'width:25%'}, 'Last update'));
	
	foreach my $db (@dbs)
	{
		my $db_id = $db->{id};
		
		next if $db_id eq 'all';
		
		my $d;
		
		eval { $d = new MRS::MDatabank($db_id); };
		next if ($@);

		my $last_update = 0;
		foreach my $f (split(m/[\|+]/, $db_id))
		{
			my $sb = stat("${mrs_data}/$f.cmp");
			next unless defined $sb;
			$last_update = $sb->mtime if $sb->mtime > $last_update;
		}
		
		my $db_id_esc = $db_id;
		$db_id_esc =~ s/\+/%2B/;
		
		my $vers = $d->GetVersion();
		$vers = undef
			if substr($vers, 0, length('no version information available')) eq 'no version information available';
		
		push @rows, $q->Tr(
			$q->td($q->a({-href=>"mrs.cgi?dbinfo=$db_id_esc"}, $db->{name})),
			$q->td($q->a({-href=>"mrs.cgi?dbinfo=$db_id_esc"}, $db_id)),
			$q->td({-style=>"text-align:right;"}, &printNumber($d->Count)),
			$q->td($vers),
			$q->td({-style=>"text-align:right;"}, scalar localtime $last_update)
		);
	}
	
	print $q->div({-class=>'list'},
		$q->table({-cellspacing=>'0', -cellpadding=>'0'}, @rows)),
		$q->br;
}

sub printDbInfo($)
{
	my $db = shift;
	
	my @rows;

	my $n = $dbInfo{$db}->{name};
	my $p = $dbInfo{$db}->{parser};
	my $f = $dbInfo{$db}->{filter};
	my $u = $dbInfo{$db}->{url};
	
	push @rows, $q->caption("Statistics for $db");
	push @rows, $q->Tr($q->th('Name'), $q->th({-style=>"text-align:right;"}, 'Value'));

	my $d = new MRS::MDatabank($db) or next;
	
	push @rows, $q->Tr($q->td('Name'), $q->td({-style=>"text-align:right;"}, $n));
	push @rows, $q->Tr($q->td('Number of records'), $q->td({-style=>"text-align:right;"}, &printNumber($d->Count)));
	push @rows, $q->Tr($q->td('Parser script'), $q->td({-style=>"text-align:right;"}, $q->a({-href=>"mrs.cgi?parser=$p"}, $p)));

	my $ff = $f;
	$ff = $q->a({-href=>"mrs.cgi?formatter=Format_$f.pm"}, "Format_$f.pm") if $f ne 'default';

	push @rows, $q->Tr($q->td('Formatting script'), $q->td({-style=>"text-align:right;"}, $ff));
	push @rows, $q->Tr($q->td('More information'), $q->td({-style=>"text-align:right;"}, $q->a({-href=>$u}, $u)));

	my (@parts, @names);

	my $nr = 0;
	foreach my $f (split(m/[\|+]/, $db))
	{
		$d = new MRS::MDatabank($f) or next;
		push @parts, $d;
		push @names, $f;

		my $sb = stat("${mrs_data}/$f.cmp");
		next unless defined $sb;
		
		++$nr;
		
		push @rows, $q->Tr($q->td("File $nr"), $q->td({-style=>"text-align:right;"}, "$f.cmp"));
		push @rows, $q->Tr($q->td("&nbsp;File size (bytes)"), $q->td({-style=>"text-align:right;"}, &printNumber($sb->size)));
		push @rows, $q->Tr($q->td("&nbsp;Number of records"), $q->td({-style=>"text-align:right;"}, &printNumber($d->Count)));
		push @rows, $q->Tr($q->td("&nbsp;Last updated"), $q->td({-style=>"text-align:right;"}, scalar localtime $sb->mtime));
	}

	print $q->div({-class=>'list'},
		$q->table({-cellspacing=>'0', -cellpadding=>'0'}, @rows)),
		$q->br;

	# display info for each part
	foreach $d (@parts)
	{
		@rows = undef;
		my $name = shift @names;

		push @rows, $q->Tr(
			$q->th(['Index kind', 'Index code', 'Index name', 'Number of entries']));
		
		my $i = $d->Indices or next;
		
		while (my $ix = $i->Next)
		{
			my $iCode = $ix->Code;
			my $iName = Format::field_name($dbFilters{$name}, $iCode);
			my $iCount = $ix->Count;
			my $iType = $index_types{$ix->Type};
			
			push @rows, $q->Tr(
				$q->td([$iType, $iCode, $iName]),
				$q->td({-style=>"text-align:right;"}, &printNumber($iCount)));
		}

		print
			$q->a({-name=>$name}),
			$q->div({-class=>'list'},
				$q->table({-cellspacing=>'0', -cellpadding=>'0'},
					$q->caption("Index listing for $name.cmp"),
					@rows
				)
			),
			$q->br;
	}
}

sub printScript($$)
{
	my ($title, $f) = @_;
	my $script;
	
	if (-f $f)
	{
		local($/) = undef;
		open IN, "<$f";
		$script = <IN>;
		close IN;
	}
	
	die "Could not open script $f" unless defined $script;
	
	print $q->h3($title);
	
	$script =~ s/&/\&amp;/g;
	$script =~ s/</&lt;/g;
	$script =~ s/>/&gt;/g;
	
	print $q->code($q->pre($script));
}

sub printHelp($)
{
	my $chapter = shift;
	
	my $text;
	
	open TXT, "<help-$chapter.html";
	local($/) = undef;
	$text = <TXT>;
	close TXT;
	
	print $q->div({class=>'help'}, $text);
}

sub getFields($$)
{
	my ($d, $db) = @_;

	my $i = $d->Indices;
	do
	{
		$fldLabels{$i->Code} = Format::field_name(&Filter($db), $i->Code);
	}
	while ($i->Next);

	push @fields, "all";
	push @fields, sort { $fldLabels{$a} cmp $fldLabels{$b} } keys %fldLabels;

	$fldLabels{"all"} = "Any Index";
}

sub main()
{
	my $db = $q->param('db');
	
#	$db = 'embl_release|embl_updates' if $db eq 'embl';

	my $first = $q->param('first');
	$first = 0 unless defined $first;

	my $count = $q->param('count');
	$count = 15 unless defined $count;

	my $format = $q->param('format');

	my $query = $q->param('query');
	
	if (not defined $query and defined $q->param('fld'))
	{
		$query = "";
		
		my @ix = $q->param('idx');
		my @fl = $q->param('fld');
		
		foreach my $i (0 .. scalar(@ix) - 1)
		{
			if (defined $ix[$i] and defined $fl[$i] and length($fl[$i]))
			{
				foreach my $f (split(/[ ,]/, $fl[$i]))
				{
					$query .= "$ix[$i]:" if $ix[$i] ne 'all';
					$query .= $f;
					$query .= " ";
				}
			}
		}
	}
	
	my $wildcard = 0;
	
	if (defined $q->param('wildcard'))
	{
		$wildcard = 1 if $q->param('wildcard') eq 'on';
	}
	
	if ($q->param('extended'))
	{
		$db = 'uniprot' if not defined $db or length($db) == 0;
		shift @dbIds;
	}
	elsif (not defined $db or length($db) == 0)
	{
		$db = 'all';
	}
	
	$db = 'uniprot' if $db eq 'sptr';
	$db = 'refseq' if $db eq 'refseqp';
	
	my $d;
	
	if (defined $db and
		($q->param('exp') or
			($q->Accept('text/plain') == 1.0 and $q->Accept('text/html') < 1.0)))
	{
		die 'Select a databank first before using the Save button'
			if (not defined $db or $db eq 'all');

		$d = new MRS::MDatabank($db) or die "Could not open databank $db";

		my $save_to = $q->param('save_to');

#		print $q->header($save_to);
		if ($q->param('save_to') eq 'text/plain') {
			print $q->header(-type=>'text/plain');
		}
		else {
			# perl CGI sucks
			print "Content-Type: application/octet-stream; charset=ISO-8859-1\n";
			print "Content-Disposition: attachement; filename=mrs_data.txt\n";
			print "\n";
		}

		
		if (my @ids = $q->param('id'))
		{
			foreach my $id (@ids)
			{
				if (defined $format and $format eq 'fasta')
				{
					print Format::fasta(&Filter($db), $q, $d->Get($id))
				}
				else
				{
					print $d->Get($id);
				}
			}
		}
		elsif ($query)
		{
			if (my $i = $d->Find($query, $wildcard))
			{
				for (my $id = $i->Next; defined $id; $id = $i->Next)
				{
					if (defined $format and $format eq 'fasta')
					{
						print Format::fasta(&Filter($db), $q, $d->Get($id))
					}
					else
					{
						print $d->Get($id);
					}
				
					print "\n";
				}
			}
		}
		
		exit;
	}
	
	my $onLoad = undef;
	if ($q->param('extended'))
	{
		$onLoad = "msrLoad(\"$q->url({-full=>1})\");";
	}

	my %cookies = fetch CGI::Cookie;
	my $cookie = $cookies{wildcard};
	$cookie = new CGI::Cookie(-name => 'wildcard', -value => $wildcard, -expires => '+1y')
		unless defined $cookie;
	$cookie->expires('+1y');

	my %html_options = (
		-title=>'Mrs',
		-script=>{'src'=>$q->base_url . "/mrs.js"},
		-style=>{'src'=>$q->base_url . "/mrs_style.css"}
	);

	$html_options{'-onLoad'} = $onLoad if defined $onLoad;

	print
		$q->header(-cookie=>[$cookie], -charset=>'utf-8'),
		$q->start_html(%html_options),
		$q->menu(),
		'<div id="main">';

		# the search bar
		
	my $searchBox;
	
	if ($q->param('extended'))
	{
		print $q->p('This page is a work in progress, it is not intended for normal use.',
			'Please use the regular search instead unless you know how this page works.',
			'An improved version of this page will follow shortly.');
		
		if (defined $db and $db ne 'all')
		{
			$d = new MRS::MDatabank($db) or die "Could not open databank $db";

			&getFields($d, $db) if defined $d;
		}
		
		$searchBox = $q->table({id=>'tabel'},
			$q->Tr(
				$q->td({colspan=>3},
					"Databank",
					&getDbPopup('uniprot', "changeDb(\"".$q->url({-relative=>1})."\");"),
				)
			),
			$q->Tr(
				$q->td({id=>'rij', colspan=>3},
					$q->popup_menu(-name=>"idx",
						-values=>\@fields,
						-default=>'all',
						-labels=>\%fldLabels
					),
					$q->textfield(
						-name=>"fld",
						-class=>'tb',
						-size=>50,
						-maxlength=>256,
						-onKeyPress=>'doKeyPress(event);'
					)
				)
			),
			$q->Tr(
				$q->td({width=>'80%'},
					$q->button(-name=>'add', -value=>"More Fields", -class=>'submit', onClick=>'addField();')
				),
				$q->td(
					$q->button(-name=>'search', -value=>"Search", -class=>'submit', onClick=>'doSearch();')
				),
#				$q->td(
#					$q->button(-name=>'clear', -value=>"Reset", -class=>'submit', onClick=>'doClear();')
#				),
			)
		);
	}
	else
	{
		$searchBox = $q->span(
			&getDbPopup('all', "doSearch();"),
#			$q->popup_menu(-name=>"db",
#				-values=>\@dbIds,
#				-default=>'all',
#				-labels=>\%dbLabels,
#				-onChange=>"doSearch();"
#			),
			"for",
			$q->textfield(
				-name=>"query",
				-class=>'tb',
				-size=>40,
				-maxlength=>256,
				-onKeyPress=>'doKeyPress(event);'
			),
			$q->button(-name=>'search', -value=>"Search", -class=>'submit', onClick=>'doSearch();'),
#			$q->button(-name=>'clear', -value=>"Reset", -class=>'submit', onClick=>'doClear();'),
		);
	}
	
	my $expires = gmtime(time + 3600 * 24 * 365);
	
	print
		$q->start_form(-name=>"f", -method=>'GET',
			-action=>$q->url({-relative=>1})), # , -onSubmit=>'return validate_input()'
		$q->div({id=>'s', class=>'blackbox'},
			"Search",
			$searchBox,
			$q->checkbox(
				-name=>'wildcard',
				-checked=>$cookie->value,
				-value=>'on',
				-label=>'Append wildcards',
				-onClick=>"setCookie('wildcard', document.f.wildcard.checked ? 1 : 0, '/', '$expires')"
			),
			$q->hidden('first', 0),
		);
	
	# main content

	if (defined $q->param('help'))
	{
		&printHelp($q->param('help'));
	}
	elsif ($q->param('dbinfo'))
	{
		&printDbInfo($q->param('dbinfo'));
	}
	elsif ($q->param('formatter'))
	{
		my $f = $q->param('formatter');
		&printScript("Formatting script $f", $f);
	}
	elsif ($q->param('parser'))
	{
		my $f = $q->param('parser');
		&printScript("Parser script $f", "$mrs_script/$f");
	}
	elsif ($q->param('overview'))
	{
		&printOverview();
	}
	elsif (not defined $q->param('id') and (not defined $query or length($query) == 0))
	{
		print $about;

		my $news;
		if (-f "news.html")
		{
			local($/) = undef;
			open IN, "<news.html";
			$news = <IN>;
			close IN;
		}
		
		if (defined $news and length($news))
		{
			print $q->h2("News"), $q->p($news);
		}
	}
	elsif ($db eq 'all')
	{
		if (defined $query)
		{
			&doListAll($query, $wildcard);
		}
	}
	else
	{
		$d = new MRS::MDatabank($db) or die "Could not open databank $db";
		
		if ($q->param('id'))
		{
			&doDisplayEntry($db, $format, $q->param('id'), $d);
		}
		elsif (defined $query)
		{
			doList($db, $format, $first, $count, $query, $wildcard, $d);
			
#			# and print some buttons to do nice things
#			
#			print
#				$q->br,
#				$q->div({id=>'b', class=>'blackbox'},
#					$q->button(
#						-name=>'save_selected',
#						-value=>"Save Selected",
#						-class=>'submit',
#						-onClick=>"doSaveSelected()"
#					),
#					$q->button(
#						-name=>'blast_selected',
#						-value=>"Blast Selected",
#						-class=>'submit',
#						-onClick=>"doBlastSelected()"
#					)
#				);
		}
	}

	print
		$q->end_form,
		"</div>",
		$q->end_html;
}
