/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "spserver/spserver.hpp"
#include "spserver/splfserver.hpp"
#include "spserver/spporting.hpp"
#include "spserver/spioutils.hpp"

#include "spnetkit/spnkendpoint.hpp"
#include "spnetkit/spnklog.hpp"
#include "spnetkit/spnksocket.hpp"

#include "spsghandler.hpp"
#include "spsgconfig.hpp"

void showUsage( const char * program )
{
	printf( "\nUsage: %s [-h <ip>] [-p <port>] [-d] [-f <config>]\n"
			"\t[-s <server mode>] [-x <loglevel>] [-o] [-d] [-v]\n", program );
	printf( "\n" );
	printf( "\t-h <ip> listen ip, default is 0.0.0.0\n" );
	printf( "\t-p <port> listen port\n" );
	printf( "\t-d run as daemon\n" );
	printf( "\t-f <config> config file, default is ./spsmtpgate.ini\n" );
	printf( "\t-s <server mode> hahs/lf, half-async/half-sync, leader/follower, default is hahs\n" );
	printf( "\t-x <loglevel> syslog level\n" );
	printf( "\t-o log socket io\n" );
	printf( "\t-v help\n" );
	printf( "\n" );

	exit( 0 );
}

int main( int argc, char * argv[] )
{
	const char * host = "0.0.0.0";
	int port = 0;
	const char * serverMode = "hahs";
	const char * configFile = "spsmtpgate.ini";
	int runAsDaemon = 0;
	int loglevel = LOG_NOTICE;

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "h:p:s:f:x:odv" )) != EOF ) {
		switch ( c ) {
			case 'h':
				host = optarg;
				break;
			case 'p':
				port = atoi( optarg );
				break;
			case 's':
				serverMode = optarg;
				break;
			case 'f':
				configFile = optarg;
				break;
			case 'o':
				SP_NKSocket::setLogSocketDefault( 1 );
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

	if( port <= 0 ) showUsage( argv[0] );

	if( runAsDaemon ) {
		if( 0 != SP_IOUtils::initDaemon() ) {
			printf( "Cannot run as daemon!\n" );
			return -1;
		}
	}

	int logopt = LOG_CONS | LOG_PID;
	if( 0 == runAsDaemon ) logopt |= LOG_PERROR;

	SP_NKLog::setLogLevel( loglevel );
	sp_openlog( "spsmtpgate", logopt, LOG_MAIL );

	SP_SGConfig config;
	config.init( configFile );

	if( 0 == strcasecmp( serverMode, "hahs" ) ) {
		SP_Server server( host, port,
				new SP_SmtpHandlerAdapterFactory( new SP_SGHandlerFactory(&config) ) );

		server.setMaxConnections( config.getMaxConnections() );
		server.setTimeout( config.getSocketTimeout() );
		server.setMaxThreads( config.getMaxThreads() );
		server.setReqQueueSize( config.getMaxReqQueueSize(), "501 Sorry, server is busy now!\n" );

		server.runForever();
	} else {
		SP_LFServer server( host, port,
				new SP_SmtpHandlerAdapterFactory( new SP_SGHandlerFactory(&config) ) );

		server.setMaxConnections( config.getMaxConnections() );
		server.setTimeout( config.getSocketTimeout() );
		server.setMaxThreads( config.getMaxThreads() );
		server.setReqQueueSize( config.getMaxReqQueueSize(), "501 Sorry, server is busy now!\n" );

		server.runForever();
	}

	sp_closelog();

	return 0;
}

