use ExtUtils::MakeMaker;

WriteMakefile(
	'NAME'				=> 'MRS',
	"VERSION"			=> "3.0.1",
	'PMLIBDIRS'			=> [q[lib]],
	'PM'				=>
	{
		'lib/MRS.pm' => '$(INST_LIBDIR)/MRS.pm',
		'lib/MRS.$(DLEXT)' => '$(INST_ARCHAUTODIR)/MRS.$(DLEXT)'
	},
	EXE_FILES			=> ['bin/mrs.pl']
);
