/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <stdio.h>

#include "spnetkit/spnkendpoint.hpp"
#include "spnetkit/spnkini.hpp"
#include "spnetkit/spnklog.hpp"
#include "spnetkit/spnkporting.hpp"
#include "spnetkit/spnkstr.hpp"
#include "spnetkit/spnklist.hpp"

#include "spnetkit/spnkmiltercli.hpp"

#include "spsgconfig.hpp"

SP_SGBackendConfig :: SP_SGBackendConfig()
{
	mServerList = new SP_NKEndPointList();
}

SP_SGBackendConfig :: ~SP_SGBackendConfig()
{
	delete mServerList, mServerList = NULL;
}

int SP_SGBackendConfig :: init( SP_NKIniFile * iniFile, const char * section )
{
	int serverCount = 0;

	SP_NKIniItemInfo_t infoArray[] = {
		SP_NK_INI_ITEM_INT( section, "ConnectTimeout", mConnectTimeout ),
		SP_NK_INI_ITEM_INT( section, "ConnectRetry", mConnectRetry ),
		SP_NK_INI_ITEM_INT( section, "SocketTimeout", mSocketTimeout ),
		SP_NK_INI_ITEM_INT( section, "ServerCount", serverCount ),

		SP_NK_INI_ITEM_END
	};

	SP_NKIniFile::BatchLoad( iniFile, infoArray );

	SP_NKIniFile::BatchDump( infoArray );

	for( int i = 0; i < serverCount; i++ ) {
		char key[ 128 ] = { 0 }, value[ 1024 ] = { 0 };

		snprintf( key, sizeof( key ), "Server%d", i );
		iniFile->getValue( section, key, value, sizeof( value ) );

		char ip[ 64 ] = { 0 }, port[ 32 ] = { 0 }, weight[ 32 ] = { 0 };

		SP_NKStr::getToken( value, 0, ip, sizeof( ip ), ':' );
		SP_NKStr::getToken( value, 1, port, sizeof( port ), ':' );
		SP_NKStr::getToken( port, 1, weight, sizeof( weight ) );

		if( '\0' == ip[0] || '\0' == port[0] || '\0' == weight[0] ) continue;

		mServerList->addEndPoint( ip, atoi( port ), atoi( weight ) );

		SP_NKLog::log( LOG_DEBUG, "[%s]%s = %s:%d %d", section,
				key, ip, atoi( port ), atoi( weight ) );
	}

	return 0;
}

void SP_SGBackendConfig :: setConnectTimeout( int connectTimeout )
{
	mConnectTimeout = connectTimeout;
}

int SP_SGBackendConfig :: getConnectTimeout()
{
	return mConnectTimeout;
}

void SP_SGBackendConfig :: setConnectRetry( int connectRetry )
{
	mConnectRetry = connectRetry;
}

int SP_SGBackendConfig :: getConnectRetry()
{
	return mConnectRetry;
}

void SP_SGBackendConfig :: setSocketTimeout( int socketTimeout )
{
	mSocketTimeout = socketTimeout;
}

int SP_SGBackendConfig :: getSocketTimeout()
{
	return mSocketTimeout;
}

SP_NKEndPointList * SP_SGBackendConfig :: getServerList()
{
	return mServerList;
}

//---------------------------------------------------------

SP_SGConfig :: SP_SGConfig()
{
	mBackend = new SP_SGBackendConfig();
	mMilterList = new SP_NKMilterListConfig();
}

SP_SGConfig :: ~SP_SGConfig()
{
	delete mBackend, mBackend = NULL;
	delete mMilterList, mMilterList = NULL;
}

int SP_SGConfig :: init( const char * configFile )
{
	int ret = 0;

	SP_NKIniFile iniFile;

	if( 0 == iniFile.open( configFile ) ) {

		SP_NKIniItemInfo_t infoArray[] = {
			SP_NK_INI_ITEM_INT( "Server", "MaxConnections", mMaxConnections ),
			SP_NK_INI_ITEM_INT( "Server", "MaxThreads", mMaxThreads ),
			SP_NK_INI_ITEM_INT( "Server", "MaxReqQueueSize", mMaxReqQueueSize ),
			SP_NK_INI_ITEM_INT( "Server", "SocketTimeout", mSocketTimeout ),
			SP_NK_INI_ITEM_END
		};

		SP_NKIniFile::BatchLoad( &iniFile, infoArray );

		if( mMaxConnections <= 0 ) mMaxConnections = 128;
		if( mMaxThreads <= 0 ) mMaxThreads = 10;
		if( mMaxReqQueueSize <= 0 ) mMaxReqQueueSize = 100;
		if( mSocketTimeout <= 0 ) mSocketTimeout = 600;

		SP_NKIniFile::BatchDump( infoArray );

		mBackend->init( &iniFile, "Backend" );

		SP_NKStringList milterNameList;
		iniFile.getKeyNameList( "MailFilter", &milterNameList );
		for( int i = 0; i < milterNameList.getCount(); i++ ) {
			const char * name = milterNameList.getItem(i);

			char value[ 512 ] = { 0 };
			iniFile.getRawValue( "MailFilter", name, value, sizeof( value ) );

			SP_NKMilterConfig * milterConfig = new SP_NKMilterConfig();
			if( 0 == milterConfig->init( name, value ) ) {
				mMilterList->append( milterConfig );
			} else {
				delete milterConfig;
				SP_NKLog::log( LOG_WARNING, "WARN: invalid milter config, %s[%s]", name, value );
			}
		}

		mMilterList->dump();

	} else {
		ret = -1;
	}

	return ret;
}

int SP_SGConfig :: getMaxConnections()
{
	return mMaxConnections;
}

int SP_SGConfig :: getSocketTimeout()
{
	return mSocketTimeout;
}

int SP_SGConfig :: getMaxThreads()
{
	return mMaxThreads;
}

int SP_SGConfig :: getMaxReqQueueSize()
{
	return mMaxReqQueueSize;
}

SP_SGBackendConfig * SP_SGConfig :: getBackend()
{
	return mBackend;
}

SP_NKMilterListConfig * SP_SGConfig :: getMilterList()
{
	return mMilterList;
}

