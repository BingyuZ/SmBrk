#include "servers.h"

#include <boost/random.hpp>

#include <memory.h>



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

uint32_t GetMyRand(bool t)
{
    static uint64_t seed = 0x9ABFF8BF139DF27Full;
    uint32_t v;
    if (t) {
        v = gRng();
        seed = (seed << 32) | v;
        return v;
    }
    else {
        seed = seed * 134775813 + 1;
        return seed >> 32;
    }
}

#if 0
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
#endif

void AgtServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << conn->localAddress().toIpPort() << " -> "
			 << conn->peerAddress().toIpPort() << " is "
			 << (conn->connected() ? "UP" : "DOWN");

	if (conn->connected())
    {
		if (numConnected_ > kMaxConn_)	// Exceed connection limit
		{
			conn->shutdown();
			LOG_WARN << "Exceeds maximum connection";
			return;
		}

        conn->setTcpNoDelay(true);

        Session sess(conn);
        //SessionPtr sess(new Session(conn));

        LOG_DEBUG << "Conn " << sess.passwd_[0] << " , " << sess.passwd_[1];
        sess.sendWelcome();

        MutexLockGuard lock(mutex_);

        ++numConnected_;
        conn->setContext(sess);
        connections_.insert(conn);
	}
	else{
        // Session &sess = boost::any_cast<Session &>(conn->getContext());
        // Make session invalid?
        MutexLockGuard lock(mutex_);

        conn->setContext(SessionPtr());

		--numConnected_;
		connections_.erase(conn);
	}

}

//  assert(!conn->getContext().empty());
//  Node* node = boost::any_cast<Node>(conn->getMutableContext());
//  node->lastReceiveTime = time;

// Returns false on error
bool AgtServer::CheckCRC(const char *c, uint32_t len)
{
    // TODO: Check CRC
    return true;
}


void AgtServer::onMessage(  const muduo::net::TcpConnectionPtr& conn,
	  	  	  				muduo::net::Buffer* buf,
	  	  	  				muduo::Timestamp t)
{
    while (buf->readableBytes() >= kMinLength + 4) // kMinLength == 16  exclude CRC
    {
		const char* pC1 = (const char *) buf->peek();

        if (*pC1 != 'Z' && *(pC1+1) != 'E') {
			LOG_ERROR << "Wrong packet head " << *pC1 << " " << *(pC1+1);
			conn->shutdown(); // FIXME: disable reading
			break;
		}

        pC1 += 2;
		uint32_t len = *pC1 * 256 + *(pC1+1);

		//LOG_DEBUG << "Packet received, len=" << len << "  Readable:" << buf->readableBytes();

		if (len < kMinLength - 4)   // Length = 12+
		{
			LOG_WARN << "Invalid packet length " << len;
			conn->shutdown();  // FIXME: disable reading
			break;
		}
		else if (buf->readableBytes() >= len + 8)   // Header + CRC
		{
		    if (!CheckCRC(pC1-2, len)) {
                LOG_WARN << "CRC Error";
                conn->shutdown();  // FIXME: disable reading
                break;
		    }

		    pC1 += 2;
		    LOG_DEBUG << "Packet received, len=" << len << " Type: " << 0L+*pC1;
		    if (*pC1 == CAA_LOGANS) {
                assert(!conn->getContext().empty());

                Session *pSess  = boost::any_cast<Session>(conn->getMutableContext());
                //SessionPtr& pSess = boost::any_cast<SessionPtr& >(conn->getMutableContext());

                //const SessionPtr& pSess = boost::any_cast<const SessionPtr& >(conn->getContext());

            LOG_DEBUG << "MSGConn " << pSess->passwd_[0] << " , " << pSess->passwd_[1];

                buf->retrieve(kHeaderLen);  // 4
                muduo::string message(buf->peek(), len);

                if (len != 16 || pSess->stage_ != SC_CHSENT || !pSess->CheckPass(message)) {
                    LOG_WARN << "Stage / password Error";
                    conn->shutdown();  // FIXME: disable reading
                    break;
                }
                buf->retrieve(len + 4);

                // TODO: Log & more work
                // If there is a previous instance, kick it out!

                pSess->GeneratePass();
                uint32_t data[4];
                for (int i=0; i<4; i++)
                    data[i] = muduo::net::sockets::networkToHost32(pSess->passwd_[i]);
                pSess->sendPacket(CAS_LOGRES, data, 16);

		    }   // CAA_LOGANS
		    else {
                // DeCrypt it

		    }

		    // TODO: Check CRC here
			//buf->retrieve(kHeaderLen);  // 4
			//muduo::string message(buf->peek(), len);
			//buf->retrieve(len + 4);
		}
		else break;	// Haven't got enough bytes
    }
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
#if 0
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
                    codec_.sendwChksum(conn, &header, 12);
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

                PacketHeader header;
                FillPacketHeader(&header, CAS_LOGRES, false);
                for (int i=0; i<4; i++)
                    header.body_[i] = gRng();
                codec_.sendwChksum(conn, &header, sizeof(header));

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
#endif
}


//#endif // 0









