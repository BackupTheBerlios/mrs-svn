use ExtUtils::MakeMaker;

WriteMakefile(
	'NAME'				=> 'MRS',
	"VERSION"			=> "2.0.5",
	'PMLIBDIRS'			=> [q[lib]],
	'PM'				=>
	{
		'lib/MRS.pm' => '$(INST_LIBDIR)/MRS.pm',
		'lib/MRS.$(DLEXT)' => '$(INST_ARCHAUTODIR)/MRS.$(DLEXT)'
	},
	EXE_FILES			=> ['bin/mrs.pl']
);
