/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spnghandler_hpp__
#define __spnghandler_hpp__

#include "spserver/spsmtp.hpp"

class SP_SGConfig;
class SP_SGMessage;
class SP_SGBackendHandler;

class SP_NKNameValueList;

class SP_SGHandler : public SP_SmtpHandler {
public:
	SP_SGHandler( SP_SGConfig * config );
	virtual ~SP_SGHandler();

	virtual int welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply );

	virtual int helo( const char * args, SP_Buffer * reply );

	virtual int ehlo( const char * args, SP_Buffer * reply );

	virtual int auth( const char * user, const char * pass, SP_Buffer * reply );

	virtual int from( const char * args, SP_Buffer * reply );

	virtual int rcpt( const char * args, SP_Buffer * reply );

	virtual int data( const char * data, SP_Buffer * reply );

	virtual int rset( SP_Buffer * reply );

private:
	static void initMacro( SP_NKNameValueList * macroList );

private:

	SP_SGConfig * mConfig;
	int mAuthResult;

	SP_SGMessage * mMessage;

	SP_NKNameValueList * mMacroList;
	SP_SmtpHandlerList * mMilterList;

	SP_SGBackendHandler * mBackend;
};

class SP_SGHandlerFactory : public SP_SmtpHandlerFactory {
public:
	SP_SGHandlerFactory( SP_SGConfig * config );
	virtual ~SP_SGHandlerFactory();

	virtual SP_SmtpHandler * create() const;

private:
	SP_SGConfig * mConfig;
};

#endif

