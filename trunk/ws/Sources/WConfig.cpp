/*
	$Id$
	
	Config parser for an mrs web service
*/

#include "HLib.h"
#include "HError.h"

#include <fstream>
#include <sstream>
#include <map>
#include <iostream>

#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "WConfig.h"

using namespace std;
namespace fs = boost::filesystem;

// default values for the directories used by mrsws, these can also be set from the Makefile

#ifndef MRS_DATA_DIR
#define MRS_DATA_DIR "/usr/local/data/mrs/"
#endif

#ifndef MRS_PARSER_DIR
#define MRS_PARSER_DIR "/usr/local/share/mrs/parser_scripts/"
#endif

#ifndef MRS_CONFIG_FILE
#define MRS_CONFIG_FILE "/usr/local/etc/mrs-config.xml"
#endif

#ifndef MRS_LOG_FILE
#define MRS_LOG_FILE "/var/log/mrsws-search.log"
#endif

#ifndef MRS_PID_FILE
#define MRS_PID_FILE "/var/run/mrsws-search.pid"
#endif

WConfigFile*	gConfig = nil;

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
							const string&	inXPath) const;
	
	bool				GetValue(
							const string&	inPath,
							vector<string>&	outValue) const;

	bool				NextServerConfig(
							string&			outService,
							string&			outAddress,
							uint16&			outPort,
							vector<string>&	outDBs);

	void				GetDbFiles(
							WDbFileInfoArray&
											outDbFiles);

	string				mPath;
	time_t				mLastModified;
	xmlDocPtr			mXMLDoc;
	xmlXPathContextPtr	mXPathContext;

	xmlXPathObjectPtr	mServers;
	xmlNodePtr			mServer;
	int					mNextServer;
};

WConfigFileImp::WConfigFileImp(
	const char*			inPath)
	: mPath(inPath)
	, mLastModified(0)
	, mXMLDoc(nil)
	, mXPathContext(nil)
	, mServers(nil)
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
	const string&		inXPath) const
{
	string result;
	
	xmlXPathObjectPtr data = xmlXPathEvalExpression((const xmlChar*)inXPath.c_str(), mXPathContext);
	xmlNodeSetPtr nodes = data->nodesetval;

	if (nodes != nil)
	{
		if (nodes->nodeNr >= 1)
		{
			xmlNodePtr node = nodes->nodeTab[0];
			const char* text = nil;
			
			if (node->children)
				text = (const char*)XML_GET_CONTENT(node->children);

			if (text != nil)
				result = text;
		}
		
		xmlXPathFreeObject(data);
	}
	
	return result;
}

bool WConfigFileImp::GetValue(
	const string&	inXPath,
	vector<string>&	outValue) const
{
	xmlXPathObjectPtr data = xmlXPathEvalExpression((const xmlChar*)inXPath.c_str(), mXPathContext);

	if (data == nil or data->nodesetval == nil)
		THROW(("Failed to locate databank information in configuration file %s", mPath.c_str()));
	
	for (int i = 0; i < data->nodesetval->nodeNr; ++i)
	{
		xmlNodePtr db = data->nodesetval->nodeTab[i];
		if (strcmp((const char*)db->name, "db"))
			continue;
		
		const char* name = (const char*)XML_GET_CONTENT(db->children);
		if (name == nil)
			continue;
		
		outValue.push_back(name);
	}
	
	if (data)
		xmlXPathFreeObject(data);
	
	return outValue.size() > 0;
}

bool WConfigFileImp::NextServerConfig(
	string&			outService,
	string&			outAddress,
	uint16&			outPort,
	vector<string>&	outDBs)
{
	bool result = false;
	
	xmlNodePtr server = nil;
	
	if (mServers == nil)
	{
		mServers = xmlXPathEvalExpression(BAD_CAST "/mrs-config/servers/server", mXPathContext);
		
		mNextServer = 0;
		for (;;)
		{
			if (mServers != nil and mServers->nodesetval != nil and mServers->nodesetval->nodeNr > 0)
			{
				server = mServers->nodesetval->nodeTab[mNextServer];
				++mNextServer;
			}
			else if (mServers != nil)
			{
				xmlXPathFreeObject(mServers);
				mServers = nil;
				break;
			}
			
			if (strcmp((const char*)server->name, "server"))
				continue;
			
			break;
		}
	}
	else if (mNextServer < mServers->nodesetval->nodeNr)
	{
		server = mServers->nodesetval->nodeTab[mNextServer];
		++mNextServer;
	}
	else
	{
		xmlXPathFreeObject(mServers);
		mServers = nil;
	}

	if (server != nil)
	{
		for (xmlNodePtr node = server->children; node != nil; node = node->next)
		{
			if (xmlNodeIsText(node) or node->children == nil)
				continue;
			
			if (strcmp((const char*)node->name, "service") == 0)
				outService = (const char*)XML_GET_CONTENT(node->children);
			else if (strcmp((const char*)node->name, "address") == 0)
				outAddress = (const char*)XML_GET_CONTENT(node->children);
			else if (strcmp((const char*)node->name, "port") == 0)
			{
				const char* port = (const char*)XML_GET_CONTENT(node->children);
				if (port != nil)
					outPort = atoi(port);
			}
		}

		stringstream xpath;
		xpath << "/mrs-config/servers/server[port=" << outPort << "]/dbs/db";
		GetValue(xpath.str(), outDBs);
		
		result = true;
	}
	
	return result;
}

void WConfigFileImp::GetDbFiles(
	WDbFileInfoArray&	outDbFiles)
{
	xmlXPathObjectPtr data = xmlXPathEvalExpression((const xmlChar*)"/mrs-config/db-files/db-file", mXPathContext);

	if (data == nil or data->nodesetval == nil)
		THROW(("Failed to locate databank information in configuration file %s", mPath.c_str()));
	
	for (int i = 0; i < data->nodesetval->nodeNr; ++i)
	{
		xmlNodePtr db = data->nodesetval->nodeTab[i];
		if (strcmp((const char*)db->name, "db-file") != 0)
			continue;
		
		WDbFileInfo info;
		
		const char* s = (const char*)xmlGetProp(db, (const xmlChar*)"id");
		if (s == nil)
			THROW(("id attribute for db-file element in config file is missing"));
		
		info.id = s;
		info.is_joined_db = false;
		
		s = (const char*)xmlGetProp(db, (const xmlChar*)"file");
		if (s == nil)
			THROW(("file attribute for db-file element in config file is missing"));
		
		info.files.push_back(s);
		
		outDbFiles.push_back(info);
	}
	
	if (data)
		xmlXPathFreeObject(data);
	
	// see if there are db-join elements, this is optional
	
	data = xmlXPathEvalExpression((const xmlChar*)"/mrs-config/db-files/db-join", mXPathContext);

	if (data != nil and data->nodesetval != nil)
	{
		for (int i = 0; i < data->nodesetval->nodeNr; ++i)
		{
			xmlNodePtr db = data->nodesetval->nodeTab[i];
			if (strcmp((const char*)db->name, "db-join") != 0)
				continue;
			
			WDbFileInfo info;
			
			const char* s = (const char*)xmlGetProp(db, (const xmlChar*)"id");
			if (s == nil)
				THROW(("id attribute for db-file element in config file is missing"));
			
			info.id = s;
			info.is_joined_db = true;
			
			for (xmlNodePtr file = db->children; file != nil; file = file->next)
			{
				if (xmlNodeIsText(file) or file->children == nil)
					continue;
				
				if (strcmp((const char*)file->name, "db-part") == 0)
				{
					s = (const char*)XML_GET_CONTENT(file->children);
					if (s != nil)
						info.files.push_back(s);
				}
			}
			
			if (info.files.size() == 0)
				THROW(("No db-part elements defined for db-join element in config file"));
			
			outDbFiles.push_back(info);
		}
	}
	
	if (data)
		xmlXPathFreeObject(data);
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

bool WConfigFile::NextServerConfig(
	string&			outService,
	string&			outAddress,
	uint16&			outPort,
	vector<string>&	outDBs)
{
	return mImpl->NextServerConfig(outService, outAddress, outPort, outDBs);
}

void WConfigFile::GetDbFiles(
	WDbFileInfoArray&	outDbFiles)
{
	mImpl->GetDbFiles(outDbFiles);
}
