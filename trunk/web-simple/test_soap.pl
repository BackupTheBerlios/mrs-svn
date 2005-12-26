#!perl -w

use SOAP::Lite;
use Data::Dumper;

use SOAP::Lite;
print Dumper(SOAP::Lite
    -> uri('http://mrs.cmbi.ru.nl/mrs')
    -> proxy('http://mrs.cmbi.ru.nl/cgi-bin/mrs_soap.cgi')
    -> query('uniprot', 'rds_human', 1)
    -> result);

#my $r = &Mrsget('uniprot', 'rds_human') or die "no result: $!";
#print Dumper($r);
#exit;
#
#sub Mrsget
#{
#   my $ns_url = 'http://mrs.cmbi.ru.nl/mrs';
#   my $ns = 'service';
#   my $url = 'http://mrs.cmbi.ru.nl/cgi-bin/mrs_soap.cgi';
#
#   my $db = shift;
#   my $query = shift;
#   my @queries = split(/,/, $query);
#
#   my $result = &soapCall(SOAP::Data->name("$ns:query")->attr({"xmlns:$ns" => $ns_url})
#	   => (
#	       SOAP::Data->name("$ns:db")      ->type('xsd:string' => $db),
#	       SOAP::Data->name("$ns:query")   ->type("$ns:array" => \@queries),
#	       SOAP::Data->name("$ns:summary") ->type('xsd:int' => 1),
#	   ));
#
#	print Dumper($result);
#
#   my %ret;
#
#   foreach my $q (@queries)
#   {
#       my $r = shift @{$result};
#
#       # Call the 'entry' method of mrs.
#       # First parameter is the databank name.
#       # Second parameter is either a string or an array of strings containing
#       # the ids of the record requested. Result will be either a string containing
#       # one record or an array of strings containing all the records requested.
#
#       my $data = &soapCall(SOAP::Data->name("$ns:entry")->attr({"xmlns:$ns" => $ns_url})
#           => (
#               SOAP::Data->name("$ns:db")    ->type('xsd:string' => $db),
#               SOAP::Data->name("$ns:id")    ->type("$ns:array" => $r)
#           ));
#
#       foreach my $d (@{$data})
#       {
#           $ret{$q} = $d;
#       }
#   }
#   return %ret;
#}
#
#
#sub soapCall()
#{
#       my $ns_url = 'http://mrs.cmbi.ru.nl/mrs';
#       my $ns = 'service';
#       my $url = 'http://mrs.cmbi.ru.nl/cgi-bin/mrs_soap.cgi';
#       my $soap = SOAP::Lite->uri($ns_url)->proxy($url);
#   my $result = $soap->call(@_);
#
#   my $err;
#   if ($result->fault)
#   {
#       $err .= $result->faultdetail."\n"    if defined $result->faultdetail;
#       $err .= $result->faultcode."\n"        if defined $result->faultcode;
#       $err .= $result->faultstring."\n"    if defined $result->faultstring;
#       $err .= $result->faultactor."\n"    if defined $result->faultactor;
#   }
#
#   return ($result->result, $err) if defined wantarray and wantarray;
#   die $err if defined $err;
#   return $result->result;
#};
