#include "servers.h"

#include <boost/random.hpp>

#define AU_ENCRY    ENCRY_NONE

#define ENCRY_NONE  0

#if 0
AgtServer::AgtServer()
{
    //ctor
}

AgtServer::~AgtServer()
{
    //dtor
}
#endif // 0

extern boost::mt19937 gRng;

void AgtServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected() ? "UP" : "DOWN");

	MutexLockGuard lock(mutex_);

	if (conn->connected())
    {
		if (numConnected_ > kMaxConn_)	// Exceed connection limit
		{
			conn->shutdown();
			LOG_WARN << "Exceeds maximum connection";
			return;
		}
        ++numConnected_;

        Session sess;

        sess.stage_ = SC_CHSENT;
        sess.passwd_[0] = gRng();
        sess.passwd_[1] = gRng();

        conn->setContext(sess);
        LOG_WARN << "Conn " << sess.passwd_[0] << " , " << sess.passwd_[1];

        connections_.insert(conn);
	}
	else{
        // Session &sess = boost::any_cast<Session &>(conn->getContext());
		--numConnected_;
		connections_.erase(conn);
	}

}

//  assert(!conn->getContext().empty());
//  Node* node = boost::any_cast<Node>(conn->getMutableContext());
//  node->lastReceiveTime = time;

void AgtServer::onMessage(  const TcpConnectionPtr& conn,
	  	  	  				Buffer* buf,
	  	  	  				Timestamp t)
{
    const Session &sess = boost::any_cast<const Session &>(conn->getContext());

    LOG_WARN << "Last " << sess.passwd_[0] << " , " << sess.passwd_[1];

}

void AgtServer::onBlockMessage( const TcpConnectionPtr& conn,
	  	  	  					const muduo::string& message,
	  	  	  					Timestamp t)
{
    #if 0
	ConnStatus	*pCS = boost::any_cast<ConnStatus *>(conn->getContext());

	int ret = BlockMessageB(conn, message, t);
	if (ret < 0)
	{
		pCS->stage_ = MC_Closing;
		conn->shutdown();
		return;
	}
	#endif
}


//#endif // 0









