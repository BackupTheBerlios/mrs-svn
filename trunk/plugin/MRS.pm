# This file was automatically generated by SWIG
package MRS;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package MRSc;
bootstrap MRS;
package MRS;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package MRS;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package MRS;

*errstr = *MRSc::errstr;
*DUST = *MRSc::DUST;
*SEG = *MRSc::SEG;

############# Class : MRS::MStringIterator ##############

package MRS::MStringIterator;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MStringIterator(@_);
    bless $self, $pkg if defined($self);
}

*Next = *MRSc::MStringIterator_Next;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MStringIterator($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MDatabank ##############

package MRS::MDatabank;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*kWildCardString = *MRSc::MDatabank_kWildCardString;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MDatabank(@_);
    bless $self, $pkg if defined($self);
}

*Create = *MRSc::MDatabank_Create;
*Merge = *MRSc::MDatabank_Merge;
*Count = *MRSc::MDatabank_Count;
*GetCode = *MRSc::MDatabank_GetCode;
*GetVersion = *MRSc::MDatabank_GetVersion;
*GetUUID = *MRSc::MDatabank_GetUUID;
*GetName = *MRSc::MDatabank_GetName;
*GetInfoURL = *MRSc::MDatabank_GetInfoURL;
*GetScriptName = *MRSc::MDatabank_GetScriptName;
*GetSection = *MRSc::MDatabank_GetSection;
*GetFilePath = *MRSc::MDatabank_GetFilePath;
*IsUpToDate = *MRSc::MDatabank_IsUpToDate;
*GetRawDataSize = *MRSc::MDatabank_GetRawDataSize;
*DumpInfo = *MRSc::MDatabank_DumpInfo;
*DumpIndex = *MRSc::MDatabank_DumpIndex;
*PrefetchDocWeights = *MRSc::MDatabank_PrefetchDocWeights;
*CountForKey = *MRSc::MDatabank_CountForKey;
*Find = *MRSc::MDatabank_Find;
*Match = *MRSc::MDatabank_Match;
*MatchRel = *MRSc::MDatabank_MatchRel;
*BooleanQuery = *MRSc::MDatabank_BooleanQuery;
*RankedQuery = *MRSc::MDatabank_RankedQuery;
*Get = *MRSc::MDatabank_Get;
*GetMetaData = *MRSc::MDatabank_GetMetaData;
*GetDescription = *MRSc::MDatabank_GetDescription;
*ContainsBlastIndex = *MRSc::MDatabank_ContainsBlastIndex;
*Sequence = *MRSc::MDatabank_Sequence;
*Blast = *MRSc::MDatabank_Blast;
*Index = *MRSc::MDatabank_Index;
*Indices = *MRSc::MDatabank_Indices;
*SuggestCorrection = *MRSc::MDatabank_SuggestCorrection;
*SetStopWords = *MRSc::MDatabank_SetStopWords;
*StoreMetaData = *MRSc::MDatabank_StoreMetaData;
*Store = *MRSc::MDatabank_Store;
*IndexText = *MRSc::MDatabank_IndexText;
*IndexTextAndNumbers = *MRSc::MDatabank_IndexTextAndNumbers;
*IndexWord = *MRSc::MDatabank_IndexWord;
*IndexValue = *MRSc::MDatabank_IndexValue;
*IndexWordWithWeight = *MRSc::MDatabank_IndexWordWithWeight;
*IndexDate = *MRSc::MDatabank_IndexDate;
*IndexNumber = *MRSc::MDatabank_IndexNumber;
*AddSequence = *MRSc::MDatabank_AddSequence;
*FlushDocument = *MRSc::MDatabank_FlushDocument;
*Finish = *MRSc::MDatabank_Finish;
*CreateDictionary = *MRSc::MDatabank_CreateDictionary;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MDatabank($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MBooleanQuery ##############

package MRS::MBooleanQuery;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Not = *MRSc::MBooleanQuery_Not;
*Union = *MRSc::MBooleanQuery_Union;
*Intersection = *MRSc::MBooleanQuery_Intersection;
*Perform = *MRSc::MBooleanQuery_Perform;
*Prefetch = *MRSc::MBooleanQuery_Prefetch;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MBooleanQuery(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MBooleanQuery($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MRankedQuery ##############

package MRS::MRankedQuery;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*AddTerm = *MRSc::MRankedQuery_AddTerm;
*AddTermsFromText = *MRSc::MRankedQuery_AddTermsFromText;
*SetAllTermsRequired = *MRSc::MRankedQuery_SetAllTermsRequired;
*SetMaxReturn = *MRSc::MRankedQuery_SetMaxReturn;
*SetAlgorithm = *MRSc::MRankedQuery_SetAlgorithm;
*Perform = *MRSc::MRankedQuery_Perform;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MRankedQuery(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MRankedQuery($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MQueryResults ##############

package MRS::MQueryResults;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Next = *MRSc::MQueryResults_Next;
*Score = *MRSc::MQueryResults_Score;
*Skip = *MRSc::MQueryResults_Skip;
*Count = *MRSc::MQueryResults_Count;
*Blast = *MRSc::MQueryResults_Blast;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MQueryResults(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MQueryResults($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MKeys ##############

package MRS::MKeys;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Next = *MRSc::MKeys_Next;
*Skip = *MRSc::MKeys_Skip;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MKeys(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MKeys($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MIndex ##############

package MRS::MIndex;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Code = *MRSc::MIndex_Code;
*Type = *MRSc::MIndex_Type;
*Count = *MRSc::MIndex_Count;
*Keys = *MRSc::MIndex_Keys;
*FindKey = *MRSc::MIndex_FindKey;
*GetIDF = *MRSc::MIndex_GetIDF;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MIndex(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MIndex($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MIndices ##############

package MRS::MIndices;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Next = *MRSc::MIndices_Next;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MIndices(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MIndices($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MBlastHit ##############

package MRS::MBlastHit;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Id = *MRSc::MBlastHit_Id;
*Title = *MRSc::MBlastHit_Title;
*Hsps = *MRSc::MBlastHit_Hsps;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MBlastHit(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MBlastHit($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MBlastHits ##############

package MRS::MBlastHits;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*ReportInXML = *MRSc::MBlastHits_ReportInXML;
*Next = *MRSc::MBlastHits_Next;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MBlastHits(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MBlastHits($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MBlastHsp ##############

package MRS::MBlastHsp;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Score = *MRSc::MBlastHsp_Score;
*BitScore = *MRSc::MBlastHsp_BitScore;
*Expect = *MRSc::MBlastHsp_Expect;
*Identity = *MRSc::MBlastHsp_Identity;
*Positive = *MRSc::MBlastHsp_Positive;
*Gaps = *MRSc::MBlastHsp_Gaps;
*QueryStart = *MRSc::MBlastHsp_QueryStart;
*SubjectStart = *MRSc::MBlastHsp_SubjectStart;
*SubjectLength = *MRSc::MBlastHsp_SubjectLength;
*QueryAlignment = *MRSc::MBlastHsp_QueryAlignment;
*SubjectAlignment = *MRSc::MBlastHsp_SubjectAlignment;
*Midline = *MRSc::MBlastHsp_Midline;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MBlastHsp(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MBlastHsp($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : MRS::MBlastHsps ##############

package MRS::MBlastHsps;
@ISA = qw( MRS );
%OWNER = ();
%ITERATORS = ();
*Next = *MRSc::MBlastHsps_Next;
sub new {
    my $pkg = shift;
    my $self = MRSc::new_MBlastHsps(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MRSc::delete_MBlastHsps($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package MRS;

*gErrStr = *MRSc::gErrStr;
*VERBOSE = *MRSc::VERBOSE;
*THREADS = *MRSc::THREADS;
*COMPRESSION = *MRSc::COMPRESSION;
*COMPRESSION_LEVEL = *MRSc::COMPRESSION_LEVEL;
*WEIGHT_BIT_COUNT = *MRSc::WEIGHT_BIT_COUNT;
*COMPRESSION_DICTIONARY = *MRSc::COMPRESSION_DICTIONARY;
*MDatabank_kWildCardString = *MRSc::MDatabank_kWildCardString;
1;
