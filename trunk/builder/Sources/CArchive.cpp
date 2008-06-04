/*
cd ~/projects/mrs/builder/
c++ -o builder builder-test.cpp -larchive -lboost_filesystem

./builder -i /data/raw/pmc/articles.tar.gz

*/

#include <boost/filesystem.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <getopt.h>
#include <string>
#include <iostream>

using namespace std;
namespace fs = boost::filesystem;

void usage()
{
	cout << "usage ..." << endl;
	exit(1);
}

void error(const char* msg, ...)
{
	cout << msg << endl;
	exit(1);
}

class Archive
{
  public:
					Archive(
						const fs::path&		inArchive);

	virtual			~Archive();
	
	class Entry
	{
	  public:

		int			read(
						void*				buffer,
						size_t				size)		{ return archive_read_data(archive, buffer, size); }

		int			skip()								{ return archive_read_data_skip(archive); }
		
		size_t		size() const						{ return archive_entry_size(entry); }
		
		string		path() const
		{
			string result;
			const char* p;
			if ((p = archive_entry_pathname(entry)) != NULL)
				result = p;
			return result;
		}
	
	  private:
		friend class Archive;

					Entry(
						struct archive*			inArchive,
						struct archive_entry*	inEntry)
						: archive(inArchive)
						, entry(inEntry)
					{}

		struct archive*			archive;
		struct archive_entry*	entry;
	};
	
	class EntryIterator : public boost::iterator_facade<EntryIterator,
									Entry, boost::forward_traversal_tag>
	{
		friend class boost::iterator_core_access;
		
	  public:
					EntryIterator(
						struct archive*			inArchive)
						: archive(inArchive)
						, e(NULL)
					{
						int err = archive_read_next_header(archive, &entry);
						if (err != ARCHIVE_OK)
						{
							archive = NULL;
							entry = NULL;
						}
						else
							e = new Entry(archive, entry);
					}

					EntryIterator()
						: archive(NULL)
						, entry(NULL)
						, e(NULL)
					{
					}

					EntryIterator(
						const EntryIterator&	rhs)
						: archive(rhs.archive)
						, entry(rhs.entry)
						, e(NULL)
					{
					}
					
		EntryIterator&
					operator=(
						const EntryIterator&	rhs)
					{
						archive = rhs.archive;
						entry = rhs.entry;
						
						delete e;
						e = NULL;
						
						return *this;
					}

	  private:
		void		increment()
					{
						delete e;
						e = NULL;
						
						if (archive_read_next_header(archive, &entry) != ARCHIVE_OK)
						{
							archive = NULL;
							entry = NULL;
						}
						else
							e = new Entry(archive, entry);
					}

		bool		equal(
						const EntryIterator&	rhs) const
					{
						return archive == rhs.archive and entry == rhs.entry;
					}
					
		reference	dereference() const
					{
						assert(e);
						return *e;
					}
		
		struct archive*				archive;
		struct archive_entry*		entry;
		Entry*						e;
	};
	
	EntryIterator	begin()			{ return EntryIterator(mArchive); }
	EntryIterator	end()			{ return EntryIterator(); }

  private:
	struct archive*	mArchive;
};

Archive::Archive(
	const fs::path&		inArchive)
	: mArchive(NULL)
{
	mArchive = archive_read_new();
	if (mArchive == NULL)
		error("Error creating archive");
	
	int err = archive_read_support_compression_all(mArchive);

	if (err == ARCHIVE_OK)
		err = archive_read_support_format_all(mArchive);
	
	if (err == ARCHIVE_OK)
		err = archive_read_open_filename(mArchive, inArchive.string().c_str(), 1024);
	
	if (err != ARCHIVE_OK)
		error(archive_error_string(mArchive));
}

Archive::~Archive()
{
	if (mArchive != NULL)
		archive_read_close(mArchive);
}

int main(int argc, char* const argv[])
{
	string file;
	
	int c;
	
	while ((c = getopt(argc, argv, "i:")) != -1)
	{
		switch (c)
		{
			case 'i':
				file = optarg;
				break;
			
			default:
				usage();
		}
	}

	if (file.length() == 0)
		usage();

	fs::path p(file);
	if (not fs::exists(p))
		error("archive does not exist");

	Archive a(p);
	size_t st = 0;
	
	for (Archive::EntryIterator e = a.begin(); e != a.end(); ++e)
	{
		size_t s = e->size();
		
		cout << e->path() << '\t' << s << endl;
		
		st += s;
		
		e->skip();
	}
	
	cout << "Total data size is " << st << endl;
	
	return 0;
}
