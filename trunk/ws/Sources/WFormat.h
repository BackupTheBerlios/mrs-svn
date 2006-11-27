/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 13:58:37
*/

#ifndef WFORMAT_H
#define WFORMAT_H

class WFormatTable
{
  public:
	std::string				Format(
								const std::string&	inFormatDir,
								const std::string&	inFormatter,
								const std::string&	inFormat,
								const std::string&	inText,
								const std::string&	inDb,
								const std::string&	inId);

	static WFormatTable&	Instance();

  private:
							WFormatTable();
							~WFormatTable();

	struct WFormatTableImp*	mImpl;
};

#endif // WFORMAT_H
