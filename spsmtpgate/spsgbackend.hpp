/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spsgbackend_hpp__
#define __spsgbackend_hpp__

#include "spserver/spsmtp.hpp"

class SP_SGConfig;

class SP_NKSocket;
class SP_NKSmtpProtocol;

class SP_SGBackendHandler : public SP_SmtpHandler {
public:
	SP_SGBackendHandler( SP_SGConfig * config );
	virtual ~SP_SGBackendHandler();

	virtual int welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply );

	virtual int helo( const char * args, SP_Buffer * reply );

	virtual int ehlo( const char * args, SP_Buffer * reply );

	virtual int auth( const char * user, const char * pass, SP_Buffer * reply );

	virtual int from( const char * args, SP_Buffer * reply );

	virtual int rcpt( const char * args, SP_Buffer * reply );

	virtual int data( const char * data, SP_Buffer * reply );

	virtual int rset( SP_Buffer * reply );

private:

	SP_SGConfig * mConfig;

	SP_NKSocket * mBackendSocket;
	SP_NKSmtpProtocol * mBackendSmtp;
};

#endif

