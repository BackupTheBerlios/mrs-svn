#!/usr/bin/perl

use warnings;
use strict;
use SOAP::Lite;# +trace => [qw(all)];
use Data::Dumper;

my $ns_url = 'http://mrs.cmbi.ru.nl/mrsws-admin';
my $ns = 'service';
my $url = 'http://localhost:8084/mrsws-admin';

my $soap = SOAP::Lite->uri($ns_url)->proxy($url);

&setDatabankInfo('sprot', 'mirroring', 0.5, 'my_new_file.tgz');
&getDatabankInfo('sprot');
exit;

sub soapCall()
{
    my $result = $soap->call(@_);
    
    my $err;
    if ($result->fault)
    {
        $err .= $result->faultdetail."\n"	if defined $result->faultdetail;
        $err .= $result->faultcode."\n"		if defined $result->faultcode;
        $err .= $result->faultstring."\n"	if defined $result->faultstring;
        $err .= $result->faultactor."\n"	if defined $result->faultactor;
    }

    die $err if defined $err;

    return ($result->paramsall, $err) if defined wantarray and wantarray;
    return $result;
};

sub setDatabankInfo
{
	my ($db, $status, $progress, $message) = @_;
	
	&soapCall(SOAP::Data->name("$ns:SetDatabankStatusInfo")
						->attr({"xmlns:$ns" => $ns_url})
		=> (
			SOAP::Data->name("$ns:db")->type('xsd:string' => $db),
			SOAP::Data->name("$ns:status")->type("$ns:DatabankStatus" => $status),
			SOAP::Data->name("$ns:progress")->type('xsd:float' => $progress),
			SOAP::Data->name("$ns:message")->type('xsd:string' => $message)
		));
};

sub getDatabankInfo
{
	my ($db, $status, $progress, $message) = @_;
	
	my $result = &soapCall(SOAP::Data->name("$ns:GetDatabankStatusInfo")
						->attr({"xmlns:$ns" => $ns_url})
		=> (
			SOAP::Data->name("$ns:db")->type('xsd:string' => $db)
		));
	
	print Dumper($result->paramsall);
}
