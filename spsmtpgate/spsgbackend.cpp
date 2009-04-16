/*
 * Copyright 2009 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>

#include "spnetkit/spnksocket.hpp"
#include "spnetkit/spnksmtpcli.hpp"
#include "spnetkit/spnkendpoint.hpp"
#include "spnetkit/spnklist.hpp"
#include "spnetkit/spnkstr.hpp"

#include "spserver/spbuffer.hpp"
#include "spserver/sputils.hpp"

#include "spsgbackend.hpp"
#include "spsgconfig.hpp"

SP_SGBackendHandler :: SP_SGBackendHandler( SP_SGConfig * config )
{
	mConfig = config;

	mBackendSocket = NULL;
	mBackendSmtp = NULL;
}

SP_SGBackendHandler :: ~SP_SGBackendHandler()
{
	if( NULL != mBackendSmtp ) {
		mBackendSmtp->quit();
		delete mBackendSmtp, mBackendSmtp = NULL;
	}

	if( NULL != mBackendSocket ) delete mBackendSocket, mBackendSocket = NULL;
}

int SP_SGBackendHandler :: welcome( const char * clientIP, const char * serverIP, SP_Buffer * reply )
{
	SP_SGBackendConfig * backend = mConfig->getBackend();

	for( int i = 0; i < backend->getConnectRetry(); i++ ) {
		const SP_NKEndPoint_t * endpoint = backend->getServerList()->getRandomEndPoint();
		if( NULL == endpoint ) break;

		mBackendSocket = new SP_NKTcpSocket( endpoint->mIP, endpoint->mPort,
				backend->getConnectTimeout() );

		if( mBackendSocket->getSocketFd() >= 0 ) {
			mBackendSocket->setSocketTimeout( backend->getSocketTimeout() );
			break;
		} else {
			delete mBackendSocket, mBackendSocket = NULL;
		}
	}

	int ret = eClose;

	if( NULL != mBackendSocket ) {
		mBackendSmtp = new SP_NKSmtpProtocol( mBackendSocket, "backend" );

		if( 0 == mBackendSmtp->welcome() ) {
			reply->append( mBackendSmtp->getLastReply() );

			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: helo( const char * args, SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->helo( args ) ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}
			reply->append( mBackendSmtp->getLastReply() );
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: ehlo( const char * args, SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		SP_NKStringList replyList;

		if( 0 == mBackendSmtp->ehlo( args, &replyList ) ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}

			for( int i = 0; i < replyList.getCount(); i++ ) {
				reply->append( replyList.getItem(i) );
				reply->append( "\n" );
			}
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: auth( const char * user, const char * pass, SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->auth( user, pass ) ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}
			reply->append( mBackendSmtp->getLastReply() );
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: from( const char * args, SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->mail( args ) ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}
			reply->append( mBackendSmtp->getLastReply() );
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: rcpt( const char * args, SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->rcpt( args ) ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}
			reply->append( mBackendSmtp->getLastReply() );
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: data( const char * data, SP_Buffer * reply )
{
	int ret = eAccept;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->data() ) {
			reply->append( mBackendSmtp->getLastReply() );

			if( ( ! mBackendSmtp->isPositiveCompletionReply() )
					&& ( ! mBackendSmtp->isPositiveIntermediateReply() ) ) {
				ret = eReject;
			}
		} else {
			ret = eClose;
			reply->append( "421 Service temporarily unavailable\r\n" );
		}

		if( eAccept == ret ) {
			if( 0 == mBackendSmtp->mailData( data, strlen( data ) ) ) {
				if( mBackendSmtp->isPositiveCompletionReply() ) {
					ret = eAccept;
				}  else {
					ret = eReject;
				}

				reply->reset();
				reply->append( mBackendSmtp->getLastReply() );
			} else {
				ret = eClose;
				reply->append( "421 Service temporarily unavailable\r\n" );
			}
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

int SP_SGBackendHandler :: rset( SP_Buffer * reply )
{
	int ret = eClose;

	if( NULL != mBackendSmtp ) {
		if( 0 == mBackendSmtp->rset() ) {
			if( mBackendSmtp->isPositiveCompletionReply() ) {
				ret = eAccept;
			}  else {
				ret = eReject;
			}
			reply->append( mBackendSmtp->getLastReply() );
		} else {
			reply->append( "421 Service temporarily unavailable\r\n" );
		}
	} else {
		reply->append( "421 Service temporarily unavailable\r\n" );
	}

	return ret;
}

