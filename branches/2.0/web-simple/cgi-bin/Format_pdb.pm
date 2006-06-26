# $Id: Format_pdb.pm,v 1.1 2005/08/30 06:49:58 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_pdb;

use Data::Dumper;

our @ISA = "Format";

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'pdb',
		@_
	};
	my $result = bless $self, "Format_pdb";
	return $result;
}

my $jmol_script=<<END;
<script src="/mrs/jmol/Jmol.js">
</script>
<script language="JavaScript">
	jmolInitialize("../jmol");
	jmolCheckBrowser("popup", "/browsercheck", "onClick");
	
	jmolApplet(250, "load %s?db=pdb&amp;id=%s&amp;exp=1&amp;save_to=text%%2Fplain;" +
		" cpk off; wireframe off; cartoon; color cartoon structure; spin");
</script>
END

sub pp
{
	my ($this, $q, $text, $id) = @_;
	
	my $script = sprintf($jmol_script, $q->url(), $id);
	
	return $script . $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc = "";
	
	if ($text =~ /^HEADER\s+(.+)/mo)
	{
		$desc = lc($1);
	}
	
	return $desc;
}

1;
