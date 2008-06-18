/*
	$Id$
	
	Config parser for an mrs web service, basically a simplified Perl parser...
*/

#ifndef WCONFIG_H
#define WCONFIG_H

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

struct WDbFileInfo
{
	std::string					id;
	bool						is_joined_db;
	std::vector<std::string>	files;
};

typedef std::vector<WDbFileInfo>	WDbFileInfoArray;

class WConfigFile
{
  public:
					WConfigFile(
						const char*		inPath);

					~WConfigFile();
	
	bool			ReloadIfModified();
	
	bool			GetSetting(
						const char*		inXPath,
						std::string&	outValue) const;

	bool			GetSetting(
						const char*		inXPath,
						long&			outValue) const;

	bool			NextServerConfig(
						std::string&	outService,
						std::string&	outAddress,
						uint16&			outPort,
						std::vector<std::string>&
										outDBs);

	void			GetDbFiles(
						WDbFileInfoArray&
										outDbFiles);

	std::string		operator[](
						const char*		inXPath) const;

  private:
	struct WConfigFileImp*	mImpl;
};

extern WConfigFile*	gConfig;

extern boost::filesystem::path
	gDataDir, gParserDir, gConfigFile, gLogFile;

inline std::string WConfigFile::operator[](
	const char*	inXPath) const
{
	std::string v;
	GetSetting(inXPath, v);
	return v;
}

#endif
