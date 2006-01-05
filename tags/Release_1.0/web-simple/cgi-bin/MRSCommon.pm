# Common routines for the MRS interface

package MRSCommon;

sub printLeftSide($)
{
	my ($q, $base_url, @buttons) = @_;

	my @links;

	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/mrs.cgi?overview=1"}, "Overview of indexed databanks"));
	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/mrs.cgi"}, "Search"));
	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/mrs.cgi?extended=1"}, "Extended Search"));
#	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/blast.cgi"}, "Blast"));
#	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/result.cgi"}, "Blast results"));
	push @links, $q->li($q->a({-href=>"$base_url/cgi-bin/mrs.cgi?help=0"}, "Help"));
	push @links, $q->li($q->a({-href=>"$base_url/download/index.html"}, "Download"));

	return $q->div({-id=>'side'}, $q->ul(@links, @buttons));
}

1;
