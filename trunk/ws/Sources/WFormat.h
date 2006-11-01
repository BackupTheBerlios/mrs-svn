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
								const std::string&	inParser,
								const std::string&	inText);
	
	static WFormatTable&	Instance();

  private:
							WFormatTable();
							~WFormatTable();

	struct WFormatTableImp*	mImpl;
};

#endif // WFORMAT_H
