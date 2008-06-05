#include "MRS.h"

#include "HError.h"

#include "CParser.h"
#include "CReader.h"
#include "CDocument.h"

#include <cmath>
#include <cstdlib>

#define PERL_NO_SHORT_NAMES 1
//#define PERL_IMPLICIT_CONTEXT 1

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

using namespace std;

// ------------------------------------------------------------------

#ifndef pTHX_
#define pTHX_
#endif

#undef SvIV
#define SvIV(sv) (SvIOK(sv) ? SvIVX(sv) : Perl_sv_2iv(aTHX_ sv))

#undef SvUV
#define SvUV(sv) (SvIOK(sv) ? SvUVX(sv) : Perl_sv_2uv(aTHX_ sv))

#undef SvNV
#define SvNV(sv) (SvNOK(sv) ? SvNVX(sv) : Perl_sv_2nv(aTHX_ sv))

#undef GvHVn
#define GvHVn(gv)	(GvGP(gv)->gp_hv ? \
			 GvGP(gv)->gp_hv : \
			 GvGP(Perl_gv_HVadd(aTHX_ gv))->gp_hv)


extern "C"
{
void xs_init(pTHX);
void boot_DynaLoader(pTHX_ CV* cv);
}

// ------------------------------------------------------------------

struct CParserImp
{
						CParserImp(
							const string&		inScriptName);

						~CParserImp();

	void				Parse(
							CReader&			inReader,
							CDocumentBuffer&	inDocBuffer);

	
	// implemented callbacks
	
	bool				GetLine(
							string&				outLine);
	
	void				IndexText(
							const char*			inIndex,
							const char*			inText);

	void				IndexWord(
							const char*			inIndex,
							const char*			inText);

	void				IndexTextAndNumbers(
							const char*			inIndex,
							const char*			inText);

	void				IndexDate(
							const char*			inIndex,
							const char*			inText);

	void				IndexNumber(
							const char*			inIndex,
							const char*			inText);

	void				IndexValue(
							const char*			inIndex,
							const char*			inText);

	void				StoreMetaData(
							const char*			inField,
							const char*			inValue);
	
	void				AddSequence(
							const char*			inSequence);
	
	void				Store(
							const char*			inDocument);
	
	void				FlushDocument();	

	// Perl interface routines

	static const char*	kMRSParserType;
	static const char*	kMRSParserObjectName;

	static CParserImp*	GetObject(
							SV*					inScalar);

	HV*					GetHash()				{ return mParserHash; }
	
	PerlInterpreter*	mPerl;
	SV*					mParser;
	HV*					mParserHash;
	static CParserImp*	sConstructingImp;

	CReader*			mReader;
	CDocumentBuffer*	mDocBuffer;
	CDocumentPtr		mCurrentDocument;
};

const char* CParserImp::kMRSParserType = "MRS::Parser";
const char* CParserImp::kMRSParserObjectName = "MRS::Parser::Object";
CParserImp* CParserImp::sConstructingImp = nil;

CParserImp::CParserImp(
	const string&		inScriptName)
	: mPerl(nil)
	, mParser(nil)
	, mParserHash(nil)
	, mReader(nil)
	, mDocBuffer(nil)
{
	mPerl = perl_alloc();
	if (mPerl == nil)
		THROW(("error allocating perl interpreter"));
	
	perl_construct(mPerl);
	
	PL_origalen = 1;
	
	PERL_SET_CONTEXT(mPerl);
	
	const char* embedding[] = { "", "MRSParser.pm" };
	
	mParserHash = Perl_newHV(aTHX);
	
	int err = perl_parse(mPerl, xs_init, 2, const_cast<char**>(embedding), nil);
	SV* errgv = GvSV(PL_errgv);

	if (err != 0)
	{
		string errmsg = "no perl error available";
	
		if (SvPOK(errgv))
			errmsg = string(SvPVX(errgv), SvCUR(errgv));
		
		THROW(("Error parsing MRSParser.pm module: %s", errmsg.c_str()));
	}
	
	SV** sp = PL_stack_sp;
	Perl_push_scope(aTHX);
	Perl_save_int(aTHX_ (int*)&PL_tmps_floor);
	PL_tmps_floor = PL_tmps_ix;
	
	if (++PL_markstack_ptr == PL_markstack_max)
	    Perl_markstack_grow(aTHX);
    *PL_markstack_ptr = (sp) - PL_stack_base;
	
	*++sp = Perl_newSVpv(aTHX_ inScriptName.c_str(), inScriptName.length());
	PL_stack_sp = sp;
	
	int n;
	static boost::mutex sGlobalGuard;
	{
		boost::mutex::scoped_lock lock(sGlobalGuard);
		
		sConstructingImp = this;
		n = Perl_call_pv(aTHX_ "MRS::load_parser", G_SCALAR | G_EVAL);
		sConstructingImp = nil;
	}

	sp = PL_stack_sp;

	errgv = GvSV(PL_errgv);
	if (n != 1 or Perl_sv_2bool(aTHX_ ERRSV))
	{
		string errmsg(SvPVX(errgv), SvCUR(errgv));
		THROW(("Perl error creating parser object: %s", errmsg.c_str()));
	}

	mParser = Perl_newRV(aTHX_ *sp);
	
	if (PL_tmps_ix > PL_tmps_floor)
		Perl_free_tmps(aTHX);

	Perl_pop_scope(aTHX);
}

CParserImp::~CParserImp()
{
	PL_perl_destruct_level = 0;
	perl_destruct(mPerl);
	perl_free(mPerl);
}

void CParserImp::Parse(
	CReader&			inReader,
	CDocumentBuffer&	inDocBuffer)
{
	mReader = &inReader;
	mDocBuffer = &inDocBuffer;
	
	mCurrentDocument.reset(new CDocument);
	
	PERL_SET_CONTEXT(mPerl);

	SV** sp = PL_stack_sp;
	Perl_push_scope(aTHX);
	Perl_save_int(aTHX_ (int*)&PL_tmps_floor);
	PL_tmps_floor = PL_tmps_ix;
	
	if (++PL_markstack_ptr == PL_markstack_max)
	    Perl_markstack_grow(aTHX);
    *PL_markstack_ptr = (sp) - PL_stack_base;
	
	*++sp = SvRV(mParser);

	PL_stack_sp = sp;
	
	Perl_call_method(aTHX_ "Parse", G_SCALAR | G_EVAL);

	SV* errgv = GvSV(PL_errgv);

	if (Perl_sv_2bool(aTHX_ ERRSV))
	{
		string errmsg(SvPVX(errgv), SvCUR(errgv));
		THROW(("Error parsing file: %s", errmsg.c_str()));
	}

	if (PL_tmps_ix > PL_tmps_floor)
		Perl_free_tmps(aTHX);

	Perl_pop_scope(aTHX);
	
	mReader = nil;
	mDocBuffer = nil;
}

CParserImp* CParserImp::GetObject(
	SV*					inScalar)
{
	CParserImp* result = nil;
	
	if (SvGMAGICAL(inScalar))
		Perl_mg_get(aTHX_ inScalar);
	
	if (Perl_sv_isobject(aTHX_ inScalar))
	{
		SV* tsv = SvRV(inScalar);

		IV tmp = 0;

		if (SvTYPE(tsv) == SVt_PVHV)
		{
			if (SvMAGICAL(tsv))
			{
				MAGIC* mg = Perl_mg_find(aTHX_ tsv, 'P');
				if (mg != nil)
				{
					inScalar = mg->mg_obj;
					if (Perl_sv_isobject(aTHX_ inScalar))
						tmp = SvIV(SvRV(inScalar));
				}
			}
		}
		else
			tmp = SvIV(SvRV(inScalar));
		
		if (tmp != 0 and strcmp(kMRSParserType, HvNAME(SvSTASH(SvRV(inScalar)))) == 0)
			result = reinterpret_cast<CParserImp*>(tmp);
	}
	
	return result;
}

// ------------------------------------------------------------------

bool CParserImp::GetLine(
	string&				outLine)
{
	return mReader->GetLine(outLine);
}

void CParserImp::IndexText(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexText(inIndex, inText);
}

void CParserImp::IndexWord(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexText(inIndex, inText);
}

void CParserImp::IndexTextAndNumbers(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexText(inIndex, inText);
}

void CParserImp::IndexDate(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexText(inIndex, inText);
}

void CParserImp::IndexValue(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexValue(inIndex, inText);
}

void CParserImp::IndexNumber(
	const char*			inIndex,
	const char*			inText)
{
	mCurrentDocument->AddIndexText(inIndex, inText);
}

void CParserImp::StoreMetaData(
	const char*			inField,
	const char*			inValue)
{
	mCurrentDocument->SetMetaData(inField, inValue);
}

void CParserImp::AddSequence(
	const char*			inSequence)
{
	mCurrentDocument->AddSequence(inSequence);
}

void CParserImp::Store(
	const char*			inDocument)
{
	mCurrentDocument->SetText(inDocument);
}

void CParserImp::FlushDocument()
{
	mDocBuffer->Put(mCurrentDocument);
	mCurrentDocument.reset(new CDocument);
}

// ------------------------------------------------------------------

XS(_MRSParser_new)
{
	dXSARGS;
	
	SV* obj = Perl_newSV(aTHX_ 0);
	HV* hash = Perl_newHV(aTHX);
	
	Perl_sv_setref_pv(aTHX_ obj, "MRS::Parser", CParserImp::sConstructingImp);
	HV* stash = SvSTASH(SvRV(obj));
	
	GV* gv = *(GV**)Perl_hv_fetch(aTHX_ stash, "OWNER", 5, true);
	if (not isGV(gv))
		Perl_gv_init(aTHX_ gv, stash, "OWNER", 5, false);
	
	HV* hv = GvHVn(gv);
	Perl_hv_store_ent(aTHX_ hv, obj, Perl_newSViv(aTHX_ 1), 0);
	
	Perl_sv_magic(aTHX_ (SV*)hash, (SV*)obj, 'P', Nullch, 0);
	Perl_sv_free(aTHX_ obj);

	SV* sv = Perl_sv_newmortal(aTHX);

	SV* self = Perl_newRV_noinc(aTHX_ (SV*)hash);
	Perl_sv_setsv_flags(aTHX_ sv, self, SV_GMAGIC);
	Perl_sv_free(aTHX_ (SV*)self);
	Perl_sv_bless(aTHX_ sv, stash);
	
	// copy the hash values into our hash
	
	for (int i = 1; i < items; i += 2)
	{
		SV* key = ST(i);
		SV* value = ST(i + 1);
		
		SvREFCNT_inc(value);
		
		HE* e = Perl_hv_store_ent(aTHX_
			CParserImp::sConstructingImp->GetHash(), key, value, 0);
		if (e == nil)
			Perl_sv_free(aTHX_ value);
	}
	
	ST(0) = sv;
	
	XSRETURN(1);
}

XS(_MRSParser_FIRSTKEY)
{
	dXSARGS;

	if (items != 1)
		Perl_croak(aTHX_ "Usage: MRS::Parser::FIRSTKEY(self);");

	CParserImp* proxy = CParserImp::GetObject(ST(0));
	
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");

	(void)Perl_hv_iterinit(aTHX_ proxy->GetHash());
	HE* e = Perl_hv_iternext(aTHX_ proxy->GetHash());
	if (e == nil)
	{
		ST(0) = Perl_sv_newmortal(aTHX);
		Perl_sv_setsv_flags(aTHX_ ST(0), &PL_sv_undef, SV_GMAGIC);
	}
	else
		ST(0) = Perl_hv_iterkeysv(aTHX_ e);

	XSRETURN(1);
}

XS(_MRSParser_NEXTKEY)
{
	dXSARGS;
	
	if (items < 1)
		Perl_croak(aTHX_ "Usage: MRS::Parser::NEXTKEY(self);");

	CParserImp* proxy = CParserImp::GetObject(ST(0));
	
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");

	HE* e = Perl_hv_iternext(aTHX_ proxy->GetHash());
	if (e == nil)
	{
		ST(0) = Perl_sv_newmortal(aTHX);
		Perl_sv_setsv_flags(aTHX_ ST(0), &PL_sv_undef, SV_GMAGIC);
	}
	else
		ST(0) = Perl_hv_iterkeysv(aTHX_ e);

	XSRETURN(1);
}

XS(_MRSParser_FETCH)
{
	dXSARGS;

	if (items != 2)
		Perl_croak(aTHX_ "Usage: MRS::Parser::FETCH(self, name);");

	CParserImp* proxy = CParserImp::GetObject(ST(0));
	
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");

	SV* key = ST(1);

	HE* e = Perl_hv_fetch_ent(aTHX_ proxy->GetHash(), key, 0, 0);
	
	SV* result;
	
	if (e != nil)
	{
		result = HeVAL(e);
		SvREFCNT_inc(result);
	}
	else
	{
		result = Perl_sv_newmortal(aTHX);
		Perl_sv_setsv_flags(aTHX_ result, &PL_sv_undef, SV_GMAGIC);
	}
	
	ST(0) = result;
	XSRETURN(1);
}

XS(_MRSParser_STORE)
{
	dXSARGS;

	if (items != 3)
		Perl_croak(aTHX_ "Usage: MRS::Parser::STORE(self, name, value);");

	CParserImp* proxy = CParserImp::GetObject(ST(0));
	
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");

	SV* key = ST(1);
	SV* value = ST(2);
	
	SvREFCNT_inc(value);
	
	HE* e = Perl_hv_store_ent(aTHX_ proxy->GetHash(), key, value, 0);
	if (e == nil)
		Perl_sv_free(aTHX_ value);

	XSRETURN(0);
}

XS(_MRSParser_CLEAR)
{
	dXSARGS;
	
	if (items < 1)
		Perl_croak(aTHX_ "Usage: MRS::Parser::NEXTKEY(self);");

	CParserImp* proxy = CParserImp::GetObject(ST(0));
	
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	Perl_hv_clear(aTHX_ proxy->GetHash());

	XSRETURN(0);
}


XS(_MRSParser_GetLine)
{
	dXSARGS;
	
	if (items != 1)
		Perl_croak(aTHX_ "Usage: MRS::Parser::GetLine(self);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	ST(0) = Perl_sv_newmortal(aTHX);

	string line;
	if (proxy->GetLine(line))
		Perl_sv_setpv(aTHX_ ST(0), line.c_str());
	else
		Perl_sv_setsv_flags(aTHX_ ST(0), &PL_sv_undef, SV_GMAGIC);
	
	XSRETURN(1);
}

XS(_MRSParser_Store)
{
	dXSARGS;
	
	if (items != 2 or not SvPOK(ST(1)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::Store(self, doc);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->Store(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)));
	
	XSRETURN(0);
}

XS(_MRSParser_AddSequence)
{
	dXSARGS;
	
	if (items != 2 or not SvPOK(ST(1)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::AddSequence(self, sequence);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->AddSequence(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)));
	
	XSRETURN(0);
}

XS(_MRSParser_StoreMetaData)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) or not SvPOK(ST(2)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::StoreMetaData(self, field, text);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->StoreMetaData(
		Perl_sv_2pvutf8_nolen(aTHX_ ST(1)),
		Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_FlushDocument)
{
	dXSARGS;
	
	if (items != 1)
		Perl_croak(aTHX_ "Usage: MRS::Parser::FlushDocument(self);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->FlushDocument();
	
	XSRETURN(0);
}

XS(_MRSParser_IndexValue)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) or not SvPOK(ST(2)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexValue(self, indexname, value);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexValue(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_IndexText)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) or not SvPOK(ST(2)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexText(self, indexname, text);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexText(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_IndexWord)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) /*or not SvPOK(ST(2))*/)
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexWord(self, indexname, word);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexWord(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_IndexTextAndNumbers)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) or not SvPOK(ST(2)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexTextAndNumbers(self, indexname, text);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexTextAndNumbers(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_IndexDate)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) or not SvPOK(ST(2)))
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexDate(self, indexname, date);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexDate(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

XS(_MRSParser_IndexNumber)
{
	dXSARGS;
	
	if (items != 3 or not SvPOK(ST(1)) /*or not SvIOK(ST(2))*/)
		Perl_croak(aTHX_ "Usage: MRS::Parser::IndexNumber(self, indexname, number);");
	
	CParserImp* proxy = CParserImp::GetObject(ST(0));
	if (proxy == nil)
		Perl_croak(aTHX_ "Error, MRS::Parser object is not specified");
	
	proxy->IndexNumber(Perl_sv_2pvutf8_nolen(aTHX_ ST(1)), Perl_sv_2pvutf8_nolen(aTHX_ ST(2)));
	
	XSRETURN(0);
}

void xs_init(pTHX)
{
	char *file = const_cast<char*>(__FILE__);
	dXSUB_SYS;

	/* DynaLoader is a special case */
	Perl_newXS(aTHX_ const_cast<char*>("DynaLoader::boot_DynaLoader"), boot_DynaLoader, file);

	// our methods
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::new"), _MRSParser_new, file);

	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::FIRSTKEY"), _MRSParser_FIRSTKEY, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::NEXTKEY"), _MRSParser_NEXTKEY, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::FETCH"), _MRSParser_FETCH, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::STORE"), _MRSParser_STORE, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::CLEAR"), _MRSParser_CLEAR, file);

	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::GetLine"), _MRSParser_GetLine, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::Store"), _MRSParser_Store, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::AddSequence"), _MRSParser_AddSequence, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::StoreMetaData"), _MRSParser_StoreMetaData, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::FlushDocument"), _MRSParser_FlushDocument, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexValue"), _MRSParser_IndexValue, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexText"), _MRSParser_IndexText, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexWord"), _MRSParser_IndexWord, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexTextAndNumbers"), _MRSParser_IndexTextAndNumbers, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexDate"), _MRSParser_IndexDate, file);
	Perl_newXS(aTHX_ const_cast<char*>("MRS::Parser::IndexNumber"), _MRSParser_IndexNumber, file);
}

// ------------------------------------------------------------------

CParser::CParser(
	const string&		inScriptName)
	: mImpl(new CParserImp(inScriptName))
{
}

CParser::~CParser()
{
	delete mImpl;
}

void CParser::Run(
	CReaderBuffer*			inReaders,
	CDocumentBuffer*		inDocBuffer)
{
	for (;;)
	{
		CReaderPtr next = inReaders->Get();
		
		if (next == CReader::sEnd)
			break;
		
		mImpl->Parse(*next, *inDocBuffer);
	}
	
	inReaders->Put(CReader::sEnd);
	inDocBuffer->Put(CDocument::sEnd);
}

bool CParser::IsRawFile(
	const string&			inFile)
{
	return true;
}

void CParser::GetInfo(
	string&			outName,
	string&			outVersion,
	string&			outUrl,
	string&			outSection,
	vector<string>&	outMetaDataFields)
{
	HV* hash = mImpl->GetHash();
	if (hash == nil)
		THROW(("runtime error"));
	
	SV** sv = Perl_hv_fetch(aTHX_ hash, "name", 4, 0);
	if (sv != nil and SvPOK(*sv))
		outName = SvPVX(*sv);
	
	sv = Perl_hv_fetch(aTHX_ hash, "version", 7, 0);
	if (sv != nil and SvPOK(*sv))
		outVersion = SvPVX(*sv);
	
	sv = Perl_hv_fetch(aTHX_ hash, "url", 3, 0);
	if (sv != nil and SvPOK(*sv))
		outUrl = SvPVX(*sv);
	
	sv = Perl_hv_fetch(aTHX_ hash, "section", 7, 0);
	if (sv != nil and SvPOK(*sv))
		outSection = SvPVX(*sv);
	
	sv = Perl_hv_fetch(aTHX_ hash, "meta", 4, 0);
	if (sv != nil)
	{
		AV* av = nil;
	
		if (SvTYPE(*sv) == SVt_PVAV)
			av = (AV*)*sv;
		else if (SvROK(*sv))
		{
			SV* rv = SvRV(*sv);
			if (SvTYPE(rv) == SVt_PVAV)
				av = (AV*)rv;
		}
		
		if (av != nil)
		{
			for (int i = 0; i < Perl_av_len(aTHX_ av) + 1; ++i)
			{
				sv = Perl_av_fetch(aTHX_ av, i, 0);
				if (sv != nil and SvPOK(*sv))
					outMetaDataFields.push_back(SvPVX(*sv));
			}
		}
	}
}
