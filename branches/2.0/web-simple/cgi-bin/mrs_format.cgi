#!/usr/bin/perl

#print MRSFormatService::MRSFormat('x',
#	{ filter => 'swiss', id => 'RDS_HUMAN', text => "ID RDS_HUMAN\n" });
#

use XMLRPC::Transport::HTTP;

my $server = XMLRPC::Transport::HTTP::CGI
	-> dispatch_to('MRSFormatService')
	-> handle;

package MRSFormatService;

sub MRSFormat {
	my ($Refer, $PassData) = @_;
	my $params = ($PassData);

	use CGI;
	use Format;
	
	return Format::pre($params->{filter}, new CGI, $params->{text}, $params->{id}, $params->{baseurl});
}

1;
