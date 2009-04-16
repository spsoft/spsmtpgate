/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <ctype.h>

#include "spnetkit/spnkmiltercli.hpp"
#include "spnetkit/spnksocket.hpp"
#include "spnetkit/spnklist.hpp"

#include "spserver/spbuffer.hpp"

#include "spmime/spmimeaddr.hpp"
#include "spmime/spmimelite.hpp"
#include "spmime/spmimeheader.hpp"

#include "spsgmilter.hpp"
#include "spsgmsg.hpp"

SP_SGMilterHandler :: SP_SGMilterHandler( SP_NKMilterConfig * config,
		SP_NKNameValueList * macroList, SP_SGMessage * smtpMsg )
{
	mMacroList = macroList;
	mConfig = config;
	mProtocol = NULL;
	mSocket = NULL;

	mSmtpMsg = smtpMsg;

	mIsAlreadyAccept = 0;
}

SP_SGMilterHandler :: ~SP_SGMilterHandler()
{
	if( NULL != mProtocol ) {
		mProtocol->quit();
		delete mProtocol, mProtocol = NULL;
	}
	if( NULL != mSocket ) delete mSocket, mSocket = NULL;
}

int SP_SGMilterHandler :: processReply( SP_Buffer * reply )
{
	int ret = eAccept;

	switch( mProtocol->getLastRespCode() ) {
		case SP_NKMilterProtocol::eAccept:
			mIsAlreadyAccept = 1;
			break;
		case SP_NKMilterProtocol::eReject:
			ret = eReject;
			reply->append( "554 Command rejected\r\n" );
			break;
		case SP_NKMilterProtocol::eTempfail:
			ret = eReject;
			reply->append( "421 Service temporarily unavailable\r\n" );
			break;
		case SP_NKMilterProtocol::eDiscard:
			// TODO: implement discard action, currently no support
			ret = eReject;
			reply->append( "554 Command rejected\r\n" );
			break;
		case SP_NKMilterProtocol::eContinue:
			break;
		case SP_NKMilterProtocol::eReplyCode:
			reply->append( mProtocol->getLastReply()->mData );
			if( mProtocol->getLastReply()->mData[0] >= '4' ) ret = eReject;
			break;
	}

	if( mIsAlreadyAccept ) mProtocol->abort();

	return ret;
}

int SP_SGMilterHandler :: welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply )
{
	struct timeval connectTimeout;

	memset( &connectTimeout, 0, sizeof( connectTimeout ) );
	connectTimeout.tv_sec = mConfig->getConnectTimeout();

	int ret = eAccept;

	if( isdigit( mConfig->getPort()[0] ) ) {
		mSocket = new SP_NKTcpSocket( mConfig->getHost(), atoi( mConfig->getPort() ), &connectTimeout );
	} else {
		mSocket = new SP_NKTcpSocket( mConfig->getPort(), &connectTimeout );
	}

	int mpRet = 0;

	if( mSocket->getSocketFd() > 0 ) {
		mSocket->setSocketTimeout( mConfig->getSendTimeout() );

		mProtocol = new SP_NKMilterProtocol( mSocket, mMacroList );

		// 0x13 : only support (ADDHDRS|CHGBODY|CHGHDRS)
		mpRet = mProtocol->negotiate( 2, 0x13 );

		if( 0 == mpRet ) mpRet = mProtocol->connect( clientIP, clientIP, 25 );

		if( 0 == mpRet ) ret = processReply( reply );
	} else {
		mpRet = -1;
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: helo( const char * args, SP_Buffer * reply )
{
	int ret = eAccept;

	if( mIsAlreadyAccept ) return ret;

	int mpRet = -1;

	if( NULL != mProtocol ) {
		mpRet = mProtocol->helo( args );
		if( 0 == mpRet ) ret = processReply( reply );
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: from( const char * args, SP_Buffer * reply )
{
	int ret = eAccept;

	if( mIsAlreadyAccept ) return ret;

	int mpRet = -1;

	if( NULL != mProtocol ) {
		mpRet = mProtocol->mail( args );

		if( 0 == mpRet ) processReply( reply );
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: rcpt( const char * args, SP_Buffer * reply )
{
	int ret = eAccept;

	if( mIsAlreadyAccept ) return ret;

	int mpRet = -1;

	if( NULL != mProtocol ) {
		mpRet = mProtocol->rcpt( args );

		if( 0 == mpRet ) processReply( reply );
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: rset( SP_Buffer * reply )
{
	int ret = eAccept;

	if( mIsAlreadyAccept ) return ret;

	int mpRet = -1;

	if( NULL != mProtocol ) {
		mpRet = mProtocol->abort();
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: data( const char * data, SP_Buffer * reply )
{
	int ret = eAccept;

	if( mIsAlreadyAccept ) return ret;

	int mpRet = -1;

	if( NULL != mProtocol ) {
		mpRet = filterHeader( data, reply );
		if( 0 == mpRet ) ret = processReply( reply );

		if( 0 == mpRet && eAccept == ret ) {
			mpRet = filterBody( data, reply );
			if( 0 == mpRet ) processReply( reply );
		}
	}

	if( 0 != mpRet ) {
		ret = eReject;
		if( mConfig->isFlagReject() ) {
			reply->append( "554 Command rejected\r\n" );
		} else if( mConfig->isFlagTempfail() ) {
			reply->append( "421 Service temporarily unavailable\r\n" );
		} else {
			ret = eAccept;
		}
	}

	return ret;
}

int SP_SGMilterHandler :: filterHeader( const char * data, SP_Buffer * reply )
{
	int mpRet = 0;

	SP_MimeHeaderList headerList;

	SP_MimeLiteParser::parseHeader( data, &headerList );

	for( int i = 0; i < headerList.getCount(); i++ ) {
		const SP_MimeHeader * header = headerList.getItem( i );

		mpRet = mProtocol->header( header->getName(), header->getValue() );

		if( 0 == mpRet ) {
			if( mProtocol->isLastRespCode( SP_NKMilterProtocol::eContinue )
					|| mProtocol->isLastRespCode( SP_NKMilterProtocol::eDiscard ) ) {
				// continue
			} else {
				break;
			}
		} else {
			break;
		}
	}

	if( 0 == mpRet && mProtocol->isLastRespCode( SP_NKMilterProtocol::eContinue ) ) {
		mpRet = mProtocol->endOfHeader();
	}

	return mpRet;
}

int SP_SGMilterHandler :: filterBody( const char * data, SP_Buffer * reply )
{
	int mpRet = 0;

	int total = strlen( data );

	for( int pos = 0; pos < total; pos++ ) {
		int len = SP_NKMilterProtocol::eChunkSize;

		len = ( total - pos ) > len ? len : ( total - pos );

		mpRet = mProtocol->body( data + pos, len );

		if( 0 == mpRet ) {
			if( mProtocol->isLastRespCode( SP_NKMilterProtocol::eContinue )
					|| mProtocol->isLastRespCode( SP_NKMilterProtocol::eDiscard ) ) {
				// continue
			} else {
				break;
			}
		} else {
			break;
		}

		pos += len;
	}

	if( 0 == mpRet && mProtocol->isLastRespCode( SP_NKMilterProtocol::eContinue ) ) {
		mpRet = mProtocol->endOfBody();

		for( ; 0 == mpRet; ) {
			int modAction = 1;
			switch( mProtocol->getLastRespCode() ) {
				case SP_NKMilterProtocol::eAddRcpt:
					mSmtpMsg->getAddRcptList()->append( mProtocol->getLastReply()->mData );
					break;
				case SP_NKMilterProtocol::eDelRcpt:
					mSmtpMsg->getDelRcptList()->append( mProtocol->getLastReply()->mData );
					break;
				case SP_NKMilterProtocol::eReplBody:
					mSmtpMsg->setData( mProtocol->getLastReply()->mData );
					break;
				case SP_NKMilterProtocol::eAddHeader:
					mSmtpMsg->getAddHeaderList()->add(
						mProtocol->getReplyHeaderName(), mProtocol->getReplyHeaderValue() );
					break;
				case SP_NKMilterProtocol::eChgHeader:
				{
					char name[ 128 ] = { 0 };
					snprintf( name, sizeof( name ), "%d_%s",
						mProtocol->getReplyHeaderIndex(), mProtocol->getReplyHeaderName() );
					mSmtpMsg->getChgHeaderList()->add( name, mProtocol->getReplyHeaderValue() );
					break;
				}
				default:
					modAction = 0;
					break;
			}

			if( modAction ) {
				mpRet = mProtocol->readReply();
			} else {
				break;
			}
		}
	}

	return mpRet;
}

