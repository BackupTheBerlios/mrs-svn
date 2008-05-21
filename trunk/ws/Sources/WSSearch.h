#ifndef WSSEARCH_H
#define WSSEARCH_H

#include "WServer.h"
#include "WDatabankTable.h"
#include "MRSInterface.h"
#include "WSSearchNSH.h"

class WSSearch : public WServerT<WSSearch>
{
  public:
					WSSearch(
						const std::string&			inAddress,
						uint16						inPortNr,
						DBInfoVector&				inDbs);

	int				GetDatabankInfo(
						std::string					db,
						std::vector<struct WSSearchNS::ns__DatabankInfo>&
													info);
	
	int				GetEntry(
						std::string					db,
						std::string					id,
						enum WSSearchNS::ns__Format	format,
						std::string&				entry);

	int				GetIndices(
						std::string					db,
						std::vector<struct WSSearchNS::ns__Index>&
													indices);

	int				Find(
						std::string					db,
						std::vector<std::string>	queryterms,
						enum WSSearchNS::ns__Algorithm
													algorithm,
						bool						alltermsrequired,
						std::string					booleanfilter,
						int							resultoffset,
						int							maxresultcount,
						struct WSSearchNS::ns__FindResponse&
													response);

	int				FindSimilar(
						std::string					db,
						std::string					id,
						enum WSSearchNS::ns__Algorithm
													algorithm,
						int							resultoffset,
						int							maxresultcount,
						struct WSSearchNS::ns__FindResponse&
													response);

	int				FindAll(
						std::vector<std::string>	queryterms,
						enum WSSearchNS::ns__Algorithm
													algorithm,
						bool						alltermsrequired,
						std::string					booleanfilter,
						std::vector<struct WSSearchNS::ns__FindAllResult>&
													response);

	int				FindAllSimilar(
						std::string					db,
						std::string					id,
						enum WSSearchNS::ns__Algorithm
													algorithm,
						std::vector<struct WSSearchNS::ns__FindAllResult>&
													response);

	int				Count(
						std::string					db,
						std::string					booleanquery,
						unsigned long&				response);

	int				Cooccurrence(
						std::string					db,
						std::vector<std::string>	ids,
						float						idf_cutoff,
						int							resultoffset,
						int							maxresultcount,
						std::vector<std::string>&	terms);

	int				SpellCheck(
						std::string					db,
						std::string					queryterm,
						std::vector<std::string>&	suggestions);

  protected:

	virtual int		Serve(
						struct soap*	inSoap);

	std::string		GetTitle(
						const std::string&			db,
						const std::string&			id);
	
	std::auto_ptr<MQueryResults>
					PerformSearch(
						MDatabank&					db,
						std::vector<std::string>	queryterms,
						enum WSSearchNS::ns__Algorithm
													algorithm,
						bool						alltermsrequired,
						std::string					booleanfilter);

	void			GetDatabankInfo(
						MDatabankPtr				db,
						WSSearchNS::ns__DatabankInfo&
													info);

	WSDatabankTable	mDBs;
};

#endif
