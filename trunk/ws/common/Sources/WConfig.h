/*
	$Id$
	
	Config parser for an mrs web service, basically a simplified Perl parser...
*/

#ifndef WCONFIG_H
#define WCONFIG_H

#include <string>
#include <vector>

struct DBInfo
{
	std::string		name;
	bool			blast;
	bool			fasta;
	bool			ignore_in_all;
};

typedef std::vector<DBInfo>	DBInfoVector;

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

	bool			GetSetting(
						const char*		inXPath,
						DBInfoVector&	outValue) const;

	std::string		operator[](
						const char*		inXPath) const;

  private:
	struct WConfigFileImp*	mImpl;
};

inline std::string WConfigFile::operator[](
	const char*	inXPath) const
{
	std::string v;
	GetSetting(inXPath, v);
	return v;
}

#endif
