/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spsgmilter_hpp__
#define __spsgmilter_hpp__

#include "spserver/spsmtp.hpp"

class SP_NKMilterConfig;
class SP_NKMilterProtocol;
class SP_NKSocket;
class SP_NKNameValueList;

class SP_SGMessage;

class SP_SGMilterHandler : public SP_SmtpHandler
{
public:
	SP_SGMilterHandler( SP_NKMilterConfig * config,
			SP_NKNameValueList * macroList, SP_SGMessage * smtpMsg );
	~SP_SGMilterHandler();

	virtual int welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply );

	virtual int helo( const char * args, SP_Buffer * reply );

	virtual int from( const char * args, SP_Buffer * reply );

	virtual int rcpt( const char * args, SP_Buffer * reply );

	virtual int data( const char * data, SP_Buffer * reply );

	virtual int rset( SP_Buffer * reply );

private:
	int filterHeader( const char * data, SP_Buffer * reply );
	int filterBody( const char * data, SP_Buffer * reply );

	int processReply( SP_Buffer * reply );

private:
	SP_NKNameValueList * mMacroList;
	SP_NKMilterConfig * mConfig;
	SP_NKMilterProtocol * mProtocol;
	SP_NKSocket * mSocket;

	SP_SGMessage * mSmtpMsg;

	int mIsAlreadyAccept;
};

#endif

