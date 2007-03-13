/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
%module MRS
%include "std_string.i"

%{
#include <vector>
#include <iostream>
using namespace std;


class MDatabank;
typedef std::vector<MDatabank*> 	MDatabankArray;
typedef std::vector<std::string>	MStringArray;
%}

%typemap(in) MDatabankArray
{
	if (!SvROK($input))
		croak("Argument $argnum is not a reference.");

	AV* av = (AV*)SvRV($input);

	if (SvTYPE(av) != SVt_PVAV)
		croak("$input is not an array.");
	
	I32 len = av_len(av) + 1;
	swig_type_info* ti = SWIG_TypeQuery("MDatabank");
	
	MDatabankArray a;

	for (int i = 0; i < len; ++i)
	{
		SV** tv = av_fetch(av, i, 0);
		
		MDatabank* obj;

		if (SWIG_ConvertPtr(*tv, (void **)&obj, ti, 0) != -1)
			a.push_back(obj);
		else
			SWIG_croak("Type error in argument 2 of MDatabank::Merge. Expected an array of MDatabank objects.");
	}
	
	$1 = a;
}

%typemap(in) MStringArray
{
	if (!SvROK($input))
		croak("Argument $argnum is not a reference.");

	AV* av = (AV*)SvRV($input);

	if (SvTYPE(av) != SVt_PVAV)
		croak("$input is not an array.");
	
	I32 len = av_len(av) + 1;
	
	std::vector<std::string> a;
	
	for (int i = 0; i < len; ++i)
	{
		SV** tv = av_fetch(av, i, 0);
		
		STRLEN len;
		const char *ptr = SvPV(*tv, len);
		if (!ptr) {
		    SWIG_croak("Undefined variable in array.");
		} else {
			string s(ptr, len);
			a.push_back(s);
		}
	}
	
	$1 = a;
}

extern std::string gErrStr;

%exception {
	try {
		$action
	}

	catch (const std::exception& e) {
		gErrStr = e.what();
		SWIG_croak(e.what());
	}
	catch (...) {
		gErrStr = "Unknown exception";
		SWIG_croak("unknown exception");
	}
}

%feature("shadow") MDatabank::Find
%{
	sub Find {
		my $result = MRSc::MDatabank::Find(@_);
		MRS::MQueryResults::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::Match
%{
	sub Match {
		my $result = MRSc::MDatabank::Match(@_);
		MRS::MBooleanQuery::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::MatchRel
%{
	sub MatchRel {
		my $result = MRSc::MDatabank_MatchRel(@_);
		MRS::MBooleanQuery::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::BooleanQuery
%{
	sub BooleanQuery {
		my $result = MRSc::MDatabank_BooleanQuery(@_);
		MRS::MBooleanQuery::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::Blast
%{
	sub Blast {
		my $result = MRSc::MDatabank_Blast(@_);
		MRS::MBlastHits::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::Index
%{
	sub Index {
		my $result = MRSc::MDatabank_Index(@_);
		MRS::MIndex::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::Indices
%{
	sub Indices {
		my $result = MRSc::MDatabank_Indices(@_);
		MRS::MIndices::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MDatabank::SuggestCorrection
%{
	sub SuggestCorrection {
		my $result = MRSc::MDatabank_SuggestCorrection(@_);
		MRS::MStringIterator::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

// why do static functions not work?
%feature("shadow") MBooleanQuery::Not
%{
	sub Not {
		my $result = MRSc::MBooleanQuery_Not(@_);
		MRS::MBooleanQuery::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MBooleanQuery::Perform
%{
	sub Perform {
		my $result = MRSc::MBooleanQuery_Perform(@_);
		MRS::MQueryResults::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MRankedQuery::Perform
%{
	sub Perform {
		my $result = MRSc::MRankedQuery_Perform(@_);
		MRS::MRankedQuery::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MQueryResults::Blast
%{
	sub Blast {
		my $result = MRSc::MQueryResults_Blast(@_);
		MRS::MBlastHits::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MIndex::Keys
%{
	sub Keys {
		my $result = MRSc::MIndex_Keys(@_);
		MRS::MKeys::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MIndex::FindKey
%{
	sub FindKey {
		my $result = MRSc::MIndex_FindKey(@_);
		MRS::MKeys::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MIndices::Next
%{
	sub Next {
		my $result = MRSc::MIndices_Next(@_);
		MRS::MIndex::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MBlastHit::Hsps
%{
	sub Hsps {
		my $result = MRSc::MBlastHit_Hsps(@_);
		MRS::MBlastHsps::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MBlastHits::Next
%{
	sub Next {
		my $result = MRSc::MBlastHits_Next(@_);
		MRS::MBlastHit::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%feature("shadow") MBlastHsps::Next
%{
	sub Next {
		my $result = MRSc::MBlastHsps_Next(@_);
		MRS::MBlastHsp::ACQUIRE($result) if defined $result;
		return $result;
	}
%}

%{
#include "MRSInterface.h"
%}

%include "MRSInterface.h"

