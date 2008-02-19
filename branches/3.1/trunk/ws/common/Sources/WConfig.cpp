/*
	$Id$
	
	Config parser for an mrs web service, basically a simplified Perl parser...
*/

#include <fstream>
#include <map>

#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "WConfig.h"
#include "WError.h"

using namespace std;
namespace fs = boost::filesystem;

#define nil NULL

// --------------------------------------------------------------------
//
//	Implementation
//

struct WConfigFileImp
{
						WConfigFileImp(
							const char*		inPath);
						~WConfigFileImp();

	string				GetValue(
							const char*		inXPath) const;
	
	bool				GetValue(
							const char*		inPath,
							DBInfoVector&	outValue) const;

	string				mPath;
	time_t				mLastModified;
	xmlDocPtr			mXMLDoc;
	xmlXPathContextPtr	mXPathContext;
};

WConfigFileImp::WConfigFileImp(
	const char*			inPath)
	: mPath(inPath)
	, mLastModified(0)
	, mXMLDoc(nil)
	, mXPathContext(nil)
{
	xmlInitParser();

	mXMLDoc = xmlParseFile(inPath);
	if (mXMLDoc == nil)
		THROW(("Failed to parse mrs configuration file %s", inPath));
	
	mXPathContext = xmlXPathNewContext(mXMLDoc);
	if (mXPathContext == nil)
		THROW(("Failed to parse mrs configuration file %s (2)", inPath));
}

WConfigFileImp::~WConfigFileImp()
{
	if (mXPathContext)
		xmlXPathFreeContext(mXPathContext);
	
	if (mXMLDoc)
		xmlFreeDoc(mXMLDoc);
	
	xmlCleanupParser();
	xmlMemoryDump();
}

string WConfigFileImp::GetValue(
	const char*			inXPath) const
{
	string result;
	
	xmlXPathObjectPtr data = xmlXPathEvalExpression((const xmlChar*)inXPath, mXPathContext);
	xmlNodeSetPtr nodes = data->nodesetval;

	if (nodes != nil)
	{
		if (nodes->nodeNr >= 1)
		{
			xmlNodePtr node = nodes->nodeTab[0];
			const char* text = (const char*)XML_GET_CONTENT(node->children);

			if (text != nil)
				result = text;
		}
		
		xmlXPathFreeObject(data);
	}
	
	return result;
}

bool WConfigFileImp::GetValue(
	const char*		inXPath,
	DBInfoVector&	outValue) const
{
	xmlXPathObjectPtr data = xmlXPathEvalExpression((const xmlChar*)inXPath, mXPathContext);

	if (data == nil or data->nodesetval == nil)
		THROW(("Failed to locate databank information in configuration file %s", mPath.c_str()));
	
	for (int i = 0; i < data->nodesetval->nodeNr; ++i)
	{
		xmlNodePtr db = data->nodesetval->nodeTab[i];
		if (strcmp((const char*)db->name, "db"))
			continue;
		
		DBInfo dbi;
		
		const char* name = (const char*)XML_GET_CONTENT(db->children);
		if (name == nil)
			continue;
		
		dbi.name = name;
		
		const char* ignore = (const char*)xmlGetProp(db, (const xmlChar*)"ignore-in-all");
		dbi.ignore_in_all = ignore != nil and strcmp(ignore, "0");
		
		const char* blast = (const char*)xmlGetProp(db, (const xmlChar*)"blast");
		dbi.blast = blast != nil and strcmp(blast, "0");
		
		const char* fasta = (const char*)xmlGetProp(db, (const xmlChar*)"fasta");
		dbi.fasta = fasta != nil and strcmp(fasta, "0");
		
		outValue.push_back(dbi);
	}
	
	if (data)
		xmlXPathFreeObject(data);
	
	return outValue.size() > 0;
}

// --------------------------------------------------------------------
//
//	Interface
//

WConfigFile::WConfigFile(
	const char*		inPath)
	: mImpl(nil)
{
	fs::path configFile(inPath, fs::native);
	
	if (not fs::exists(configFile))
	{
		cerr << "Configuration file " << inPath << " does not exist, aborting" << endl;
		exit(1);
	}
	
	mImpl = new WConfigFileImp(configFile.string().c_str());
	mImpl->mLastModified = last_write_time(configFile);
}

WConfigFile::~WConfigFile()
{
	delete mImpl;
}

bool WConfigFile::ReloadIfModified()
{
	bool result = false;

	fs::path configFile(mImpl->mPath);
	
	if (fs::exists(configFile) and
		mImpl->mLastModified != last_write_time(configFile))
	{
		delete mImpl;
		mImpl = new WConfigFileImp(configFile.string().c_str());
		mImpl->mLastModified = last_write_time(configFile);
		
		result = true;
	}
	
	return result;
}

bool WConfigFile::GetSetting(
	const char*			inXPath,
	string&				outValue) const
{
	outValue = mImpl->GetValue(inXPath);
	return outValue.length() > 0;
}

bool WConfigFile::GetSetting(
	const char*			inXPath,
	long&				outValue) const
{
	string s = mImpl->GetValue(inXPath);
	outValue = atoi(s.c_str());
	return s.length() > 0;
}

bool WConfigFile::GetSetting(
	const char*		inXPath,
	DBInfoVector&	outValue) const
{
	return mImpl->GetValue(inXPath, outValue);
}
