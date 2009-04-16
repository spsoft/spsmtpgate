/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>

#include "spnetkit/spnklist.hpp"
#include "spnetkit/spnkstr.hpp"

#include "spmime/spmimemodify.hpp"

#include "spsgmsg.hpp"

SP_SGMessage :: SP_SGMessage()
{
	memset( mMsgID, 0, sizeof( mMsgID ) );

	memset( mClientIP, 0, sizeof( mClientIP ) );
	memset( mHeloArg, 0, sizeof( mHeloArg ) );

	memset( mUser, 0, sizeof( mUser ) );
	memset( mPass, 0, sizeof( mPass ) );

	memset( mFrom, 0, sizeof( mFrom ) );
	mRcptList = new SP_NKStringList();;

	mAddRcptList = new SP_NKStringList();
	mDelRcptList = new SP_NKStringList();

	mAddHeaderList = new SP_NKNameValueList();
	mChgHeaderList = new SP_NKNameValueList();

	mData = NULL;
}

SP_SGMessage :: ~SP_SGMessage()
{
	delete mRcptList, mRcptList = NULL;

	delete mAddRcptList, mAddRcptList = NULL;
	delete mDelRcptList, mDelRcptList = NULL;

	delete mAddHeaderList, mAddHeaderList = NULL;
	delete mChgHeaderList, mChgHeaderList = NULL;

	if( NULL != mData ) free( mData ), mData = NULL;
}

void SP_SGMessage :: reset()
{
	mMsgID[0] = '\0';
	mFrom[0] = '\0';

	mRcptList->clean();
	mAddRcptList->clean();
	mDelRcptList->clean();
	mAddHeaderList->clean();
	mChgHeaderList->clean();

	if( NULL != mData ) free( mData ), mData = NULL;
}

void SP_SGMessage :: setMsgID( const char * msgid )
{
	SP_NKStr::strlcpy( mMsgID, msgid, sizeof( mMsgID ) );
}

const char * SP_SGMessage :: getMsgID()
{
	return mMsgID;
}

void SP_SGMessage :: setClientIP( const char * clientIP )
{
	SP_NKStr::strlcpy( mClientIP, clientIP, sizeof( mClientIP ) );
}

const char * SP_SGMessage :: getClientIP()
{
	return mClientIP;
}

void SP_SGMessage :: setHeloArg( const char * heloArg )
{
	SP_NKStr::strlcpy( mHeloArg, heloArg, sizeof( mHeloArg ) );
}

const char * SP_SGMessage :: getHeloArg()
{
	return mHeloArg;
}

void SP_SGMessage :: setAuthInfo( const char * user, const char * pass )
{
	SP_NKStr::strlcpy( mUser, user, sizeof( mUser ) );
	SP_NKStr::strlcpy( mPass, pass, sizeof( mPass ) );
}

const char * SP_SGMessage :: getUser()
{
	return mUser;
}

const char * SP_SGMessage :: getPass()
{
	return mPass;
}

void SP_SGMessage :: setFrom( const char * from )
{
	SP_NKStr::strlcpy( mFrom, from, sizeof( mFrom ) );
}

const char * SP_SGMessage :: getFrom()
{
	return mFrom;
}

SP_NKStringList * SP_SGMessage :: getRcptList()
{
	return mRcptList;
}

SP_NKStringList * SP_SGMessage :: getAddRcptList()
{
	return mAddRcptList;
}

SP_NKStringList * SP_SGMessage :: getDelRcptList()
{
	return mDelRcptList;
}

SP_NKNameValueList * SP_SGMessage :: getAddHeaderList()
{
	return mAddHeaderList;
}

SP_NKNameValueList * SP_SGMessage :: getChgHeaderList()
{
	return mChgHeaderList;
}

void SP_SGMessage :: setData( char * data )
{
	if( NULL != mData ) free( mData );
	mData = data;
}

const char * SP_SGMessage :: getData()
{
	return mData;
}

char * SP_SGMessage :: takeData()
{
	char * ret = mData;

	mData = NULL;

	return ret;
}

char * SP_SGMessage :: applyChanges( const char * data )
{
	SP_MimeSimpleEditor editor( data );

	for( int i = 0; i < mAddHeaderList->getCount(); i++ ) {
		editor.addHeader( mAddHeaderList->getName(i), mAddHeaderList->getValue(i) );
	}

	mAddHeaderList->clean();

	for( int i = 0; i < mChgHeaderList->getCount(); i++ ) {
		const char * name = mChgHeaderList->getName(i);
		const char * value = mChgHeaderList->getName(i);

		char index[ 16 ] = { 0 };
		const char * next = NULL;
		SP_NKStr::getToken( name, 0, index, sizeof( index ), '_', &next );

		if( NULL != next ) {
			editor.removeHeader( next, atoi( index ) );
			if( '\0' != value ) {
				editor.addHeader( name, value );
			}
		}
	}

	mChgHeaderList->clean();

	return editor.getNewData();
}

