#ifndef WDATABANKTABLE_H
#define WDATABANKTABLE_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <map>

class CDatabankBase;
typedef boost::shared_ptr<CDatabankBase>		CDatabankPtr;

class WSDatabankTable
{
  public:
						WSDatabankTable();

	static WSDatabankTable&	Instance();

	CDatabankPtr		operator[](
							const std::string&		inDb);
	
	void				ReloadDbs();

  private:
						~WSDatabankTable();

						WSDatabankTable(
							const WSDatabankTable&);

	WSDatabankTable&	operator=(
							const WSDatabankTable&);

	struct WSDatabankTableImp*
						mImpl;
	boost::mutex		mMutex;
};

#endif
