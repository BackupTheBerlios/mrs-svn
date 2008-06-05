#include "MRS.h"



#include "CDocument.h"

CDocumentPtr CDocument::sEnd;

CDocument::CDocument()
{
}

CDocument::~CDocument()
{
}

void CDocument::SetText(
	const char*		inText)
{
	mText.assign(inText);
}

void CDocument::SetMetaData(
	const char*		inField,
	const char*		inText)
{
	mMetaData[inField] = inText;
}

void CDocument::AddIndexText(
	const char*		inIndex,
	const char*		inText)
{
	if (strcasecmp(inIndex, "id") == 0)
		mID.assign(inText);
	
	mIndexedTextData[inIndex] += inText;
	mIndexedTextData[inIndex] += "\n";
}

void CDocument::AddIndexValue(
	const char*		inIndex,
	const char*		inValue)
{
	mIndexedValueData[inIndex] = inValue;
}

void CDocument::AddSequence(
	const char*		inSequence)
{
	mSequences.push_back(inSequence);
}
