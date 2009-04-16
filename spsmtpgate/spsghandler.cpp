/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>

#include "spnetkit/spnklist.hpp"
#include "spnetkit/spnkmiltercli.hpp"
#include "spnetkit/spnkbase64.hpp"
#include "spnetkit/spnkstr.hpp"
#include "spnetkit/spnklog.hpp"

#include "spserver/spbuffer.hpp"
#include "spserver/sputils.hpp"

#include "spmime/spmimeaddr.hpp"
#include "spmime/spmimemodify.hpp"

#include "spsghandler.hpp"
#include "spsgmsg.hpp"
#include "spsgconfig.hpp"

#include "spsgmilter.hpp"
#include "spsgbackend.hpp"

SP_SGHandler :: SP_SGHandler( SP_SGConfig * config )
{
	mConfig = config;
	mAuthResult = 0;

	mMessage = new SP_SGMessage();

	mBackend = new SP_SGBackendHandler( mConfig );

	mMacroList = new SP_NKNameValueList();

	initMacro( mMacroList );

	mMessage->setMsgID( mMacroList->getValue( "i" ) );

	SP_NKMilterListConfig * milterListConfig = config->getMilterList();

	mMilterList = new SP_SmtpHandlerList();

	for( int i = 0; i < milterListConfig->getCount(); i++ ) {
		SP_NKMilterConfig * milterConfig = milterListConfig->getItem( i );
		SP_SGMilterHandler * handler = new SP_SGMilterHandler(
				milterConfig, mMacroList, mMessage );
		mMilterList->append( handler );
	}
}

SP_SGHandler :: ~SP_SGHandler()
{
	delete mBackend, mBackend = NULL;

	delete mMessage, mMessage = NULL;

	delete mMilterList, mMilterList = NULL;

	delete mMacroList, mMacroList = NULL;
}

void SP_SGHandler :: initMacro( SP_NKNameValueList * macroList )
{
	char msgid[ 11 ] = { 0 };
	SP_NKStr::genID( msgid, sizeof( msgid ) );
	macroList->add( "i", msgid );

	char ourhost[ 128 ] = { 0 };
	gethostname( ourhost, sizeof( ourhost ) );
	macroList->add( "j", ourhost );
	macroList->add( "w", ourhost );

	macroList->add( "{daemon_name}", "spsmtpgate" );
}

int SP_SGHandler :: welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply )
{
	int ret = eAccept;

	mMacroList->add( "{if_addr}", serverIP );

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->welcome( clientIP, serverIP, reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->welcome( clientIP, serverIP, reply );
	}

	if( eAccept == ret ) {
		mMessage->setClientIP( clientIP );
	}

	return ret;
}

int SP_SGHandler :: helo( const char * args, SP_Buffer * reply )
{
	int ret = eAccept;

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->helo( args, reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->helo( args, reply );
	}

	if( eAccept == ret ) {
		mMessage->setHeloArg( args );
	}

	return ret;
}

int SP_SGHandler :: ehlo( const char * args, SP_Buffer * reply )
{
	int ret = eAccept;

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->helo( args, reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->ehlo( args, reply );
	}

	if( eAccept == ret ) {
		mMessage->setHeloArg( args );
	}

	return ret;
}

int SP_SGHandler :: auth( const char * user, const char * pass, SP_Buffer * reply )
{
	SP_NKBase64DecodedBuffer plainUser( user, strlen( user ) );
	SP_NKBase64DecodedBuffer plainPass( pass, strlen( pass ) );

	int ret = mBackend->auth( (char*)plainUser.getBuffer(), (char*)plainPass.getBuffer(), reply );

	if( eAccept == ret ) {
		mMacroList->add( "{auth_authen}", (char*)plainUser.getBuffer() );
		mMessage->setAuthInfo( (char*)plainUser.getBuffer(), (char*)plainPass.getBuffer() );
	}

	return ret;
}

int SP_SGHandler :: from( const char * args, SP_Buffer * reply )
{
	SP_MimeAddressList addrList;
	SP_MimeAddress::parse( args, &addrList );

	SP_MimeMailboxVector * flatlist = addrList.getFlatList();

	if( flatlist->size() <= 0 ) {
		reply->append( "501 Bad address syntax\r\n" );
		return eReject;
	}

	int ret = eAccept;

	SP_MimeMailbox * mailbox = (*flatlist)[ flatlist->size() - 1 ];

	char addr[ 256 ] = { 0 };
	snprintf( addr, sizeof( addr ), "<%s>", mailbox->getMailbox() );

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->from( addr, reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->from( args, reply );
	}

	if( eAccept == ret ) {
		mMessage->setFrom( mailbox->getMailbox() );
	}

	return ret;
}

int SP_SGHandler :: rcpt( const char * args, SP_Buffer * reply )
{
	SP_MimeAddressList addrList;
	SP_MimeAddress::parse( args, &addrList );

	SP_MimeMailboxVector * flatlist = addrList.getFlatList();

	if( flatlist->size() <= 0 ) {
		reply->append( "501 Bad address syntax\r\n" );
		return eReject;
	}

	int ret = eAccept;

	SP_MimeMailbox * mailbox = (*flatlist)[ flatlist->size() - 1 ];

	char addr[ 256 ] = { 0 };
	snprintf( addr, sizeof( addr ), "<%s>", mailbox->getMailbox() );

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->rcpt( addr, reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->rcpt( args, reply );
	}

	if( eAccept == ret ) {
		mMessage->getRcptList()->append( mailbox->getMailbox() );
	}

	return ret;
}

int SP_SGHandler :: data( const char * data, SP_Buffer * reply )
{
	int ret = eAccept;

	const char * currData = data;

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->data( currData, reply );
		if( eAccept != ret ) break;

		if( NULL != mMessage->getData() ) {
			if( currData != data ) free( (char*)currData );
			currData = mMessage->takeData();
		}

		if( mMessage->getAddHeaderList()->getCount() > 0
				|| mMessage->getChgHeaderList()->getCount() > 0 ) {
			char * newData = mMessage->applyChanges( currData );

			if( currData != data ) free( (char*)currData );
			currData = newData;
		}
	}

	if( eAccept == ret ) {
		ret = mBackend->data( currData, reply );
	}

	if( eAccept == ret ) {
		SP_NKLog::log( LOG_NOTICE, "queueid=<%s>, from=<%s>, size=%d, nrcpt=%d",
				mMessage->getMsgID(), mMessage->getFrom(), strlen( currData ),
				mMessage->getRcptList()->getCount() );
	}

	if( currData != data ) free( (char*)currData );

	return ret;
}

int SP_SGHandler :: rset( SP_Buffer * reply )
{
	int ret = eAccept;

	for( int i = 0; i < mMilterList->getCount(); i++ ) {
		SP_SmtpHandler * handler = mMilterList->getItem( i );
		ret = handler->rset( reply );
		if( eAccept != ret ) break;
	}

	if( eAccept == ret ) {
		ret = mBackend->rset( reply );
	}

	mMessage->reset();

	mMacroList->remove( "i" );

	char msgid[ 11 ] = { 0 };
	SP_NKStr::genID( msgid, sizeof( msgid ) );
	mMacroList->add( "i", msgid );

	mMessage->setMsgID( msgid );

	return ret;
}

//---------------------------------------------------------

SP_SGHandlerFactory :: SP_SGHandlerFactory( SP_SGConfig * config )
{
	mConfig = config;
}

SP_SGHandlerFactory :: ~SP_SGHandlerFactory()
{
}

SP_SmtpHandler * SP_SGHandlerFactory :: create() const
{
	return new SP_SGHandler( mConfig );
}


