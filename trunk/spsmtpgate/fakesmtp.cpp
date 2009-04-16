/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "spserver/spporting.hpp"

#include "spserver/spbuffer.hpp"
#include "spserver/spserver.hpp"
#include "spserver/splfserver.hpp"
#include "spserver/sphandler.hpp"
#include "spserver/spresponse.hpp"
#include "spserver/spsmtp.hpp"
#include "spserver/spioutils.hpp"

class SP_FakeSmtpHandler : public SP_SmtpHandler {
public:
	SP_FakeSmtpHandler(){
		mAuthResult = 1;
	}

	virtual ~SP_FakeSmtpHandler() {}

	int ehlo( const char * args, SP_Buffer * reply )
	{
		reply->append( "250-OK\n" );
		reply->append( "250-AUTH LOGIN\n" );
		reply->append( "250 HELP\n" );

		return eAccept;
	}

	int auth( const char * user, const char * pass, SP_Buffer * reply )
	{
		//printf( "auth user %s, pass %s\n", user, pass );

		reply->append( "235 Authentication successful.\r\n" );
		mAuthResult = 1;

		return eAccept;
	}

	virtual int from( const char * args, SP_Buffer * reply ) {

		//printf( "mail from: %s\n", args );

		if( 0 == mAuthResult ) {
			reply->append( "503 Error: need AUTH command\r\n" );
			return eReject;
		}

		char buffer[ 128 ] = { 0 };
		snprintf( buffer, sizeof( buffer ), "250 %s, sender ok\r\n", args );
		reply->append( buffer );

		return eAccept;
	}

	virtual int rcpt( const char * args, SP_Buffer * reply ) {

		//printf( "rcpt to: %s\n", args );

		char buffer[ 128 ] = { 0 };
		snprintf( buffer, sizeof( buffer ), "250 %s, recipient ok\r\n", args );
		reply->append( buffer );

		return eAccept;
	}

	virtual int data( const char * data, SP_Buffer * reply ) {

		//printf( "data length %d\n", strlen( data ) );

		reply->append( "250 Requested mail action okay, completed.\r\n" );

		return eAccept;
	}

	virtual int rset( SP_Buffer * reply ) {
		reply->append( "250 OK\r\n" );

		return eAccept;
	}

private:
	int mAuthResult;
};

//---------------------------------------------------------

class SP_FakeSmtpHandlerFactory : public SP_SmtpHandlerFactory {
public:
	SP_FakeSmtpHandlerFactory();
	virtual ~SP_FakeSmtpHandlerFactory();

	virtual SP_SmtpHandler * create() const;

	//use default SP_CompletionHandler is enough, not need to implement
	//virtual SP_CompletionHandler * createCompletionHandler() const;
};

SP_FakeSmtpHandlerFactory :: SP_FakeSmtpHandlerFactory()
{
}

SP_FakeSmtpHandlerFactory :: ~SP_FakeSmtpHandlerFactory()
{
}

SP_SmtpHandler * SP_FakeSmtpHandlerFactory :: create() const
{
	return new SP_FakeSmtpHandler();
}

//---------------------------------------------------------

void showUsage( const char * program )
{
	printf( "\nUsage: %s [-p <port>] [-d] [-s <server mode>] "
			"[-x <loglevel>] [-d] [-v]\n", program );
	exit( 0 );
}

int main( int argc, char * argv[] )
{
	int port = 2025;
	const char * serverMode = "hahs";
	int runAsDaemon = 0;
	int loglevel = LOG_NOTICE;
	int maxThreads = 10;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "p:s:t:x:dv" )) != EOF ) {
		switch ( c ) {
			case 'p' :
				port = atoi( optarg );
				break;
			case 's':
				serverMode = optarg;
				break;
			case 't':
				maxThreads = atoi( optarg );
				break;
			case 'x':
				loglevel = atoi( optarg );
				break;
			case 'd':
				runAsDaemon = 1;
				break;
			case '?' :
			case 'v' :
				showUsage( argv[0] );
		}
	}

	if( runAsDaemon ) {
		if( 0 != SP_IOUtils::initDaemon() ) {
			printf( "Cannot run as daemon!\n" );
			return -1;
		}
	}

	int logopt = LOG_CONS | LOG_PID;
	if( 0 == runAsDaemon ) logopt |= LOG_PERROR;

	sp_openlog( "fakesmtp", logopt, LOG_USER );

	if( 0 == strcasecmp( serverMode, "hahs" ) ) {
		SP_Server server( "", port, new SP_SmtpHandlerAdapterFactory( new SP_FakeSmtpHandlerFactory() ) );

		server.setMaxConnections( 2048 );
		server.setTimeout( 600 );
		server.setMaxThreads( maxThreads );
		server.setReqQueueSize( 100, "501 Sorry, server is busy now!\n" );

		server.runForever();
	} else {
		SP_LFServer server( "", port, new SP_SmtpHandlerAdapterFactory( new SP_FakeSmtpHandlerFactory() ) );

		server.setMaxConnections( 2048 );
		server.setTimeout( 600 );
		server.setMaxThreads( maxThreads );
		server.setReqQueueSize( 100, "501 Sorry, server is busy now!\n" );

		server.runForever();
	}

	sp_closelog();

	return 0;
}

