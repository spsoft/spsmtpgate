/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spsgconfig_hpp__
#define __spsgconfig_hpp__

class SP_NKEndPointList;
class SP_NKIniFile;
class SP_NKVector;
class SP_NKMilterListConfig;

class SP_SGBackendConfig {
public:
	SP_SGBackendConfig();
	~SP_SGBackendConfig();

	int init( SP_NKIniFile * iniFile, const char * section );

	void setConnectTimeout( int connectTimeout );
	int getConnectTimeout();

	void setConnectRetry( int connectRetry );
	int getConnectRetry();

	void setSocketTimeout( int socketTimeout );
	int getSocketTimeout();

	SP_NKEndPointList * getServerList();

private:
	int mConnectTimeout;
	int mConnectRetry;
	int mSocketTimeout;

	SP_NKEndPointList * mServerList;
};

class SP_SGConfig {
public:
	SP_SGConfig();
	~SP_SGConfig();

	int init( const char * configFile );

	int getMaxConnections();
	int getSocketTimeout();
	int getMaxThreads();
	int getMaxReqQueueSize();

	SP_SGBackendConfig * getBackend();

	SP_NKMilterListConfig * getMilterList();

private:
	int mMaxConnections;
	int mSocketTimeout;
	int mMaxThreads;
	int mMaxReqQueueSize;

	SP_SGBackendConfig * mBackend;

	SP_NKMilterListConfig * mMilterList;
};

#endif

