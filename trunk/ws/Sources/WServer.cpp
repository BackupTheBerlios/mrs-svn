#include "HLib.h"

#include <cstdarg>

#include "HError.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "WServer.h"

using namespace std;

WServer::WServer(
	const std::string&			inAddress,
	uint16						inPortNr,
	SOAP_NMAC struct Namespace	inNameSpaces[])
	: mMasterSocket(-1)
	, mLog(nil)
{
	soap_init2(&mSoap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE | SOAP_C_UTFSTRING);
	
#ifndef NDEBUG
	soap_set_recv_logfile(&mSoap, "/home/maarten/projects/mrs/ws/r_mrsws.log"); // append all messages received in /logs/recv/service12.log
	soap_set_sent_logfile(&mSoap, "/home/maarten/projects/mrs/ws/s_mrsws.log"); // append all messages sent in /logs/sent/service12.log
	soap_set_test_logfile(&mSoap, "/home/maarten/projects/mrs/ws/d_mrsws.log"); // no file name: do not save debug messages
#endif
	
	soap_set_namespaces(&mSoap, inNameSpaces);
	
	mSoap.bind_flags = SO_REUSEADDR;
	mMasterSocket = soap_bind(&mSoap, inAddress.c_str(), inPortNr, 100);
	if (mMasterSocket < 0)
	{
		char b[1024];
		
		THROW(("Error binding address %s:%d; \"%s\"",
			inAddress.c_str(), inPortNr, soap_sprint_fault(&mSoap, b, sizeof(b))));
	}
}

WServer::~WServer()
{
	soap_done(&mSoap);
}

void WServer::Stop()
{
}

void WServer::Log(
	const char*		inMessage,
	...) throw ()
{
	char msg[1024];
	
	// write a nice log entry
	// start with the IP address of our client
	
	uint32 n = snprintf(msg, sizeof(msg), "%ld.%ld.%ld.%ld - %s ",
		((mSoap.ip >> 24) & 0xFF),
		((mSoap.ip >> 16) & 0xFF),
		((mSoap.ip >>  8) & 0xFF),
		( mSoap.ip        & 0xFF),
		mSoap.userid ? mSoap.userid : "-");

	char* b = msg + n;
	uint32 size = sizeof(msg) - n;
	
	// then the time of the log message
	
	time_t now;
	time(&now);
	
	struct tm tm;
	localtime_r(&now, &tm);
	
	n = strftime(b, size, "[%d/%b/%Y-%H:%M:%S] ", &tm);

	b += n;
	size -= n;
	
	n = snprintf(b, size, "%s ", mAction.c_str());
	
	b += n;
	size -= n;
	
	// and then the rest of the log message as passed by the parameters
	
	va_list vl;
	va_start(vl, inMessage);
	vsnprintf(b, size, inMessage, vl);
	va_end(vl);

	try
	{
		// finally print out the stuff to the log file and make sure we
		// get serialized output.
	
		static boost::mutex sLockLog;
		boost::mutex::scoped_lock lock(sLockLog);
		
		cout << msg << endl;
	}
	catch (...)
	{
		fputs("Exception caught in Log", stdout);
		fputs(msg, stdout);
	}
}

void WServer::Log(
	const string&		inMessage)
{
	Log(inMessage.c_str());
}

void WServer::LogFault(
	ostream&			inLogger) throw()
{
	char b[1024];
	soap_sprint_fault(&mSoap, b, sizeof(b));
	
	replace(b, b + sizeof(b), '\n', ';');
	
	for (const char* p = strtok(b, "\n\r"); p != nil; p = strtok(nil, "\n\r"))
		inLogger << p << "; ";
}

void WServer::Accept() throw ()
{
	mSoap.accept_timeout = -1;	// timeout
	mSoap.recv_timeout = 30;
	mSoap.send_timeout = 30;
	mSoap.max_keep_alive = 10;

	int s = soap_accept(&mSoap);
	if (s < 0)
	{
		if (s != SOAP_EOF)
		{
			ostringstream l;
			LogFault(l);
			Log(l.str());
		}
	}
	else
	{
		try
		{
			mSoap.user = this;
			ostringstream logger;
			mLog = &logger;
			
			for (;;)
			{
				soap_begin(&mSoap);
				
				if (soap_begin_recv(&mSoap) != SOAP_OK)
				{
					if (mSoap.error < SOAP_STOP)
						break;

					soap_closesock(&mSoap);
					continue;
				}
				
				break;
			}
			
			int err = mSoap.error;
			
			if (err == SOAP_OK)
				err = soap_envelope_begin_in(&mSoap);

			if (err == SOAP_OK)
				err = soap_recv_header(&mSoap);

			if (err == SOAP_OK)
				err = soap_body_begin_in(&mSoap);

			soap_peek_element(&mSoap);
			mAction = mSoap.tag;

			if (err == SOAP_OK)
				err = Serve(&mSoap);

			if (err != SOAP_OK)
			{
				soap_send_fault(&mSoap);
				LogFault(logger);
			}

			Log(logger.str());
		}
		catch (exception& e)
		{
			soap_receiver_fault(&mSoap,
				"Error processing command",
				e.what());

			soap_send_fault(&mSoap);
			Log("error %s", e.what());
		}
		catch (...)
		{
			soap_receiver_fault(&mSoap,
				"Error processing command",
				"unhandled exception");

			soap_send_fault(&mSoap);
			Log("error unknown exception processing SOAP message");
		}
		
		soap_destroy(&mSoap);
		soap_end(&mSoap);
	}
	
	mLog = nil;
}
