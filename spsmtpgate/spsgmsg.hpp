/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spsgmsg_hpp__
#define __spsgmsg_hpp__

class SP_NKStringList;
class SP_NKNameValueList;

class SP_SGMessage {
public:
	SP_SGMessage();
	~SP_SGMessage();

	void setMsgID( const char * msgid );
	const char * getMsgID();

	void setClientIP( const char * clientIP );
	const char * getClientIP();

	void setHeloArg( const char * heloArg );
	const char * getHeloArg();

	void setAuthInfo( const char * user, const char * pass );
	const char * getUser();
	const char * getPass();

	void setFrom( const char * from );
	const char * getFrom();

	SP_NKStringList * getRcptList();

	SP_NKStringList * getAddRcptList();
	SP_NKStringList * getDelRcptList();

	SP_NKNameValueList * getAddHeaderList();
	SP_NKNameValueList * getChgHeaderList();

	void setData( char * data );
	const char * getData();
	char * takeData();

	char * applyChanges( const char * data );

	void reset();

private:
	char mMsgID[ 32 ];

	char mClientIP[ 16 ], mHeloArg[ 128 ];

	char mUser[ 128 ], mPass[ 128 ];

	char mFrom[ 128 ];
	SP_NKStringList * mRcptList;

	SP_NKStringList * mAddRcptList;
	SP_NKStringList * mDelRcptList;

	SP_NKNameValueList * mAddHeaderList;
	SP_NKNameValueList * mChgHeaderList;

	char * mData;
};

#endif

