/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 13:58:37
*/

#ifndef WFORMAT_H
#define WFORMAT_H

class WFormatTable
{
  public:
	static std::string		Format(
								const std::string&	inFormatter,
								const std::string&	inFormat,
								const std::string&	inText,
								const std::string&	inDb,
								const std::string&	inId);

	static std::string		IndexName(
								const std::string&	inFormatter,
								const std::string&	inIndex);
	
	static void				SetParserDir(
								const std::string&	inParserDir);

  private:
							WFormatTable();
							~WFormatTable();

	static struct WFormatTableImp*
							Impl();
	static std::string		sParserDir;
};

#endif // WFORMAT_H
