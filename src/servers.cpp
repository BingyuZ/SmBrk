#include "servers.h"

#include <boost/random.hpp>

#include <memory.h>

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

void FillPacketHeader(PacketHeader *pH, enum CommandAgt cmd, bool genSalt)
{
    memset(pH, 0, sizeof(PacketHeader));

    pH->cmd_ = cmd;
    if (genSalt) {
        pH->saltHi_ = gRng();
        pH->saltLo_ = gRng();
    }
    pH->ver_ = 1;
    pH->rev_ = AU_ENCRY;
}

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

        conn->setTcpNoDelay(true);

        PacketHeader header;
        FillPacketHeader(&header, CAS_WELCOME);

        Session sess;
        sess.stage_ = SC_CHSENT;
        sess.rev_ = header.rev_;
        sess.passwd_[0] = header.saltHi_;
        sess.passwd_[1] = header.saltLo_;

        conn->setContext(sess);
        LOG_WARN << "Conn " << sess.passwd_[0] << " , " << sess.passwd_[1];

        codec_.sendwChksum(conn, &header, sizeof(header));

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

void AgtServer::DevStatus(const TcpConnectionPtr& conn,
                          const Session & sess,
                          const muduo::string& message)
{


}

void AgtServer::SaveHistory(const TcpConnectionPtr& conn,
                          const Session & sess,
                          const muduo::string& message)
{


}

void AgtServer::NewData(const TcpConnectionPtr& conn,
                          const Session & sess,
                          const muduo::string& message)
{


}

void AgtServer::DevLost(const TcpConnectionPtr& conn,
                          const Session & sess,
                          const muduo::string& message)
{


}

void AgtServer::onBlockMessage( const TcpConnectionPtr& conn,
	  	  	  					const muduo::string& message,
	  	  	  					Timestamp t)
{
    assert(!conn->getContext().empty());
    //Session* sess = boost::any_cast<Session>(conn->getMutableContext());

	const PacketHeader *pPack = reinterpret_cast<const PacketHeader *> (message.data());

	LOG_DEBUG << "Packet received! Length:" << message.length() << " CMD:" << pPack->cmd_;

	int iPos = 0;
	do {
        const Session &sess = boost::any_cast<const Session &>(conn->getContext());
        if (sess.stage_ == SC_CHSENT) {
            if (pPack->cmd_ != CAA_LOGANS || pPack->rev_ != sess.rev_
             || message.length() != 0x10 ) {
                LOG_WARN << "No Ack or auth algorithm error.";
                conn->shutdown();
                return;
            }
            // TODO: Check passwords
            // Assume everything is done
            iPos = 1;
            break;
        }
        else if (sess.stage_ != SC_PASSED) {
            LOG_WARN << "Stage error";
            conn->shutdown();
            return;
        }
        else {
            switch (pPack->cmd_) {
            case CAA_KALIVE:
                {
                    PacketHeader header;
                    FillPacketHeader(&header, CAS_KALIVE, false);
                    codec_.sendwChksum(conn, &header, sizeof(header));
                }
                break;
            case CAA_DEVSTA:
                DevStatus(conn, sess, message);
                break;
            case CAA_DEVHIS:
                SaveHistory(conn, sess, message);
                break;
            case CAA_DATA:
                NewData(conn, sess, message);
                break;
            case CAA_DEVLOST:
                DevLost(conn, sess, message);
                break;
            default:
                LOG_WARN << "Unknown command type: " << pPack->cmd_;
                //conn->shutdown();
                break;
            }
        }
	} while(0);

	if (iPos == 0) return;

	Session* sess = boost::any_cast<Session>(conn->getMutableContext());
	switch (iPos) {
	    case 1:
	        {
                const uint32_t ul = *reinterpret_cast<const uint32_t *>(pPack->body_);
                sess->agentId_ = muduo::net::sockets::networkToHost32(ul);
                sess->stage_ = SC_PASSED;
                // TODO: Log & more work
                // If there is a previous instance, kick it out!
                LOG_DEBUG << "Agent " << sess->agentId_ << " logged in";
            }
            break;
        default:
            break;
	}

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









