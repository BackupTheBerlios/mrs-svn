/*
	$Id$
	
	Config parser for an mrs web service, basically a simplified Perl parser...
*/

#include <string>
#include <vector>
#include <exception>

struct DbInfo
{
	std::string			id;
	std::string			name;
	std::string			filter;
	std::string			url;
	std::string			parser;
	bool				blast;
};

class config_exception : public std::exception
{
  public:
						config_exception(const std::string& inError, int inLine);

	virtual const char*	what() const throw()		{ return msg; }
	
  private:
	char				msg[1024];
};

void ReadConfig(const std::string& inPath, std::string& outDataDir,
	std::vector<DbInfo>& outDbInfo);
