#include "servers.h"

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


void AgtServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected() ? "UP" : "DOWN");

	MutexLockGuard lock(mutex_);

	if (conn->connected()){
		PacketHead localHeader;
		++numConnected_;
		if (numConnected_ > kMaxConn_)	// Exceed connection limit
		{
			conn->shutdown();
			LOG_WARN << "Exceeds maximum connection";
			return;
		}


	}
	else{

	}

}

void AgtServer::onBlockMessage( const TcpConnectionPtr& conn,
	  	  	  					const muduo::string& message,
	  	  	  					Timestamp t)
{
	ConnStatus	*pCS = boost::any_cast<ConnStatus *>(conn->getContext());

	int ret = BlockMessageB(conn, message, t);
	if (ret < 0)
	{
		pCS->stage_ = MC_Closing;
		conn->shutdown();
		return;
	}
}


//#endif // 0









