# Perl module voor het parsen van pdb
#
# $Id$
#
# Copyright (c) 2005
#      CMBI, Radboud University Nijmegen. All rights reserved.
#
# This code is derived from software contributed by Maarten L. Hekkelman
# and Hekkelman Programmatuur b.v.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the Radboud University
#        Nijmegen and its contributors.
# 4. Neither the name of Radboud University nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

package oxford::parser;

my $count = 0;

# code from Marc!

#############################################################
# converts a cytogenetic localisation into min/max number
# if its a range min and max of the range are returned.
#############################################################

sub cytoloc2number
{
   my $maxnrchrom = 97;
   my $chr;
   my $arm;
   my $band;
   my $subband;

   # Get the argument
   my $localisation = shift @_;
      #convert to lowercase.
   $localisation =~ tr/[A-Z]/[a-z]/;

   # Is the input a range?
   if ( $localisation =~ m/-/ ){
       ($min, $max) = &cytorange2number($localisation);
       goto RETURNVALUES;
   }
      # parse the localisation into chrnr-spec-band-subband
      if ( $localisation =~ m/^(\d+|x|y|mt)(pter|pcen|p|cen|qcen|qter|q)*(\d*)\.*(\d*)/i ){
      #build value
       $chr = $1;
       $arm = $2;
       $band = $3;
       $subband = $4;
          # if only a chromosome is given: max range.
       if ( ($chr eq $localisation) || ("${chr}pq" eq $localisation) || ($chr eq "mt") ) {
           $chr = "98" if ( $chr eq "x" );
           $chr = "99" if ( $chr eq "y" );
           if ( $chr eq "mt" ){
               $min = $max = "0";
               goto RETURNVALUES;
           }                       $min = "-${chr}29999";
           $max = "+${chr}29999";
           goto RETURNVALUES;
       }
              # chromosome arm p = +, q = -, cen = +
       if ( $arm =~ m/p|cen/ ) {
           $min = $max = "+";
          }
       elsif ( $arm =~ m/q/ ) {
           $min = $max = "-";
       }
       else {
           die ("invalid chromosome arm...$localisation\n");
       }
          # chromosome number x=98, y=99, mt=0
       if ( $chr =~ m/\d+/ ) {
           if ( $chr gt $maxnrchrom ){
               die("chromosome number out of rang...$localisation\n");
           }
           $min .= $chr;
           $max .= $chr;
       }
       elsif ( $chr eq "x" ) {
           $min .= "98";
           $max .= "98";
       }
       elsif ( $chr eq "y" ) {
           $min .= "99";
           $max .= "99";
       }
              # special notations:pter/p/q/qter=2, pcen/qcen=1, cen=0
       if ( $arm =~ m/pter|qter/ ) {
           $min .= "29998";
           $max .= "29999";
           goto RETURNVALUES;;           }           elsif ( $arm =~ m/pcen|qcen/ ) {
           $min .= "10000";
           $max .= "19999";
           goto RETURNVALUES;;           }
       elsif ( $arm =~ m/cen/ ) {
           $min .= "10000"; # change to 00000 if you want to treat cen separate from pcen/qcen
           $max .= "19999";
           goto RETURNVALUES;;           }
       else {
           $min .= "2";
           $max .= "2";
       }
              # bandnr: min=00 max=bandnr or 99
       if ( $band =~ m/\d+/) {
           if ( $band =~ m/^0\d/) { # convert 1p01 to 1p1
               $band =~ s/^0//;
           }
           if ( $band > 99 ) {
               die ("band to big...$localisation\n");
           }
           my $add = '';
           $add = "9" if ( $band < 10 );
           $min .= "$band$add";
           $max .= "$band$add";           }
       else {
              $min .= "00";
           $max .= "99";
          }
       # subbandnr: min=00 max=subbandnr or 99
       if ( $subband =~ m/\d+/ ) {
           if ( $subband =~ m/^0\d/) { # convert 1p12.01 to 1p12.1
               $subband =~ s/^0//;
           }
           if ( $subband > 99 ) {
               die("band to big...$localisation\n");
           }
           $addn = "9" if ( $subband < 10 );
           $addz = "0" if ( $subband < 10 );
           $min .= "$subband$addz";
           $max .= "$subband$addn";
              }
       else {
              $min .= "00";
           $max .= "99";
          }
       goto RETURNVALUES;;
   }
   else
   {
          die("no valid cytoloc...$localisation\n");
   }

	# conversion rules:
	# position
	# 5,6,7,8: min..max of range    (convert e.g. 2 to 29 for max, and 20 for min)
	#    1    2    3    4    5    6    7    8
	#    -arm-    ---chr---    -spec-    --band---    -subband-
	#    +=P    0=mt        2=pter    99..99        98..99
	#    -=q    1..97=chr    2=p    00..bandnr|99    00..subbandnr|99
	#        98=X        1=pcen    00..99        00..99
	#        99=Y        0=cen    00..99        00..99    (changed to 1: pcen=cen=qcen)
	#                1=qcen    00..99        00..99       #                2=q    00..bandnr|99    00..subbandnr/99
	#                2=qter    99..99        98..99
	#        mt = 0..0
	#
	RETURNVALUES:
	{
		if ( $max > $min )
		{
		   return ( $min, $max ); # switch, because the values are negative...
		}
		else {
		   return ( $max, $min );
		}
	}
}

#########################################################
# converts a cytogenetic range into min/max number
#########################################################

sub cytorange2number
{
	my $range = shift @_;
	my ($band1min, $band1max);
	my ($band2min, $band2max);
	my @list;
	
	if ( $range =~ m/(.+?)-(.+)/ )
	{
		my ($b1, $b2) = ($1, $2);
	
		($band1min, $band1max) = &cytoloc2number($b1);
		
		my ($b1chr, $b1arm);
		
		if ($b1 =~ m/^(\d+|x|y|mt)(pter|pcen|p|cen|qcen|qter|q)*(\d*)\.*(\d*)/i)
		{
			$b1chr = $1;
			$b1arm = $2;
		}
		
		if ($b2 =~ m/(.+?)(pter|pcen|p|cen|qcen|qter|q)/i)
		{
			if (not defined $1)
			{
				$b2 = "$b1chr$b2";
			}
		}	
		else
		{
			$b2 = "$b1chr$b1arm$b2";
		}
			
		($band2min, $band2max) = &cytoloc2number($b2);

		@list = sort numerically $band1min, $band1max, $band2min, $band2max;
		
		if ( $list[3] > $list[0] )
		{
			return( $list[0], $list[3] ); # switch, because the values are negative...
		}
		else
		{
			return( $list[3], $list[0] );
		}
	}
}

#########################################################
# used for sorting numerically
#########################################################

sub numerically { $a <=> $b } 


# the parser

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "oxford::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my $m = $self->{mrs};
	
	my ($doc, $last_id);
	
	$last_id = "";
	
	my $line = <IN>;
	$line = <IN> while (defined $line and substr($line, 0, 5) ne '-----');
	
	my (@offsets, $offset, $f);
	
	$f = ' ';
	
	for (my $i = 0; $i < length($line); ++$i)
	{
		if ($f ne substr($line, $i, 1))
		{
			$f = substr($line, $i, 1);
			push @offsets, $i if ($f eq '-')
		}
	}
	push @offsets, length($line);
	
	my $id = 0;
	
	while ($line = <IN>)
	{
		chomp($line);
		
		last if length($line) == 0;

		my @data;
		my $cytoloc_min = "";
		my $cytoloc_max = "";

		push @data, $id;
		$m->IndexValue('id', $id++);
		
		for (my $n = 0; $n < 10; ++$n)
		{
			my $v = substr($line, $offsets[$n], $offsets[$n + 1] - $offsets[$n]);
			
			next unless defined $v;		
			
			$v =~ s/\s+$//;
			$v =~ s/^\s+//;
			
			push @data, $v;
			
			$m->IndexText('llhid', $v) if $n == 0;
			$m->IndexTextAndNumbers('hsym', $v) if $n == 1;
			$m->IndexTextAndNumbers('loc', $v) if $n == 2;
			
			if ($n == 3)
			{
				$v = substr($v, 4) if (substr($v, 0, 4) eq 'MGI:');
				$m->IndexTextAndNumbers('acc', $v);
			}

			$m->IndexTextAndNumbers('llmid', $v) if $n == 4;
			$m->IndexTextAndNumbers('msym', $v) if $n == 5;
			$m->IndexTextAndNumbers('chr', $v) if $n == 6;
			$m->IndexTextAndNumbers('cm', $v) if $n == 7;
			$m->IndexTextAndNumbers('cb', $v) if $n == 8;
			$m->IndexTextAndNumbers('data', $v) if $n == 9;
			
			# cytoloc berekening
			
			if ($n == 2 and lc $v ne 'un')
			{
				eval
				{
					($cytoloc_min, $cytoloc_max) = cytoloc2number($v);
					
					$m->IndexWord('loc_min', $cytoloc_min);
					$m->IndexWord('loc_max', $cytoloc_max);
				};
			
				if ($@)
				{
					warn "Skipping invalid location: $v\n"
				}
			}
		}
		
		push @data, "$cytoloc_min";
		push @data, "$cytoloc_max";

		$m->Store(join("\t", @data));
		$m->FlushDocument;
	}
}

sub raw_files()
{
	my ($self, $raw_dir) = @_;
	return "<$raw_dir/HMD_Human3.rpt";
}


1;
