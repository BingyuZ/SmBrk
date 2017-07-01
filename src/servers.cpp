#include "servers.h"
#include "commons.h"
#include "redisWrapper.h"

#include <boost/random.hpp>

#include <memory.h>
#include <sys/time.h>



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

uint64_t Get64FromDevInfo(DevInfoHeader *pInfo)
{
    uint64_t r = pInfo->devType_;
    for (int i=0; i<6; i++)
        r = (r << 8) | pInfo->dID_[i];
    return r;
}

void Session::sendDevAck(DevInfoHeader *pInfo)
{
    DevInfoHeader head;

    head.type_ = DP_ACK;
    head.devType_ = pInfo->devType_;
    memcpy(head.dID_, pInfo->dID_, 6);
    head.len_ = sizeof(head);

    sendPacket(CAS_DEVANS, &head, head.len_);
}

void Session::sendLogRes(const char *pC1)
{
    uint32_t pwd[4];
    pwd[0] = passwd_[0];
    pwd[1] = passwd_[1];
    memcpy(pwd+2, pC1+4, 8);

    GeneratePass();
    uint32_t data[6];
    for (int i=0; i<4; i++)
        data[i] = muduo::net::sockets::networkToHost32(passwd_[i]);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    #if defined __x86_64__
        data[4] = muduo::net::sockets::networkToHost32(tv.tv_sec >> 32);
        data[5] = muduo::net::sockets::networkToHost32(tv.tv_sec & 0xffffffffUL);
    #else
        data[4] = 0;
        data[5] = muduo::net::sockets::networkToHost32(tv.tv_sec);
    #endif
    sendPacket(CAS_LOGRES, data, 24, pwd);
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
        //SessionPtr pSess(new Session(conn));
        sess.sendWelcome();

        MutexLockGuard lock(mutex_);

        ++numConnected_;
        conn->setContext(sess);
        connections_.insert(conn);
	}
	else
    {
        Session* pSess = boost::any_cast<Session>(conn->getMutableContext());
        pSess->ResetConn();

        MutexLockGuard lock(mutex_);
        conn->setContext(NULL);
        //conn->setContext(SessionPtr());

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

		if (len < kMinLength - 4)   // Length = 12+
		{
			LOG_WARN << "Invalid packet length " << len;
			conn->shutdown();  // FIXME: disable reading
			break;
		}
		else if (buf->readableBytes() >= len + 8)   // Header + CRC
		{
		    uint8_t cmd;
		    if (!CheckCRC(pC1-2, len)) {
                LOG_WARN << "CRC Error";
                conn->shutdown();  // FIXME: disable reading
                break;
		    }

		    pC1 += 2;
            buf->retrieve(kHeaderLen);  // 4

		    LOG_DEBUG << "Packet received, len=" << len << ", Type: " << 0L+*pC1;

            assert(!conn->getContext().empty());
            Session *pSess  = boost::any_cast<Session>(conn->getMutableContext());

            cmd = *pC1;

		    if (cmd == CAA_LOGANS) {   // Decryption not needed for LOGANS.
		        //SessionPtr *pSess = boost::any_cast<SessionPtr>(conn->getMutableContext());
                muduo::string message(buf->peek(), len);

		        // FIXME: Add information
		        // Maybe expanded in the future
                if (len != 16 || pSess->stage_ != SC_CHSENT || !pSess->CheckPass(message)) {
                    LOG_WARN << "Stage / password error";
                    conn->shutdown();  // FIXME: disable reading
                    break;
                }
                uint32_t aid = muduo::net::sockets::networkToHost32(*(uint32_t *)(pC1+12));

                buf->retrieve(len + 4);

                // TODO: Log & more work
                // If there is a previous instance, kick it out!

                LOG_INFO << "Agent " << aid << " Logged in";
                // FIXME: More info
                pSRedis_->agentLogin(aid);

                pSess->sendLogRes(pC1);
		    }   // CAA_LOGANS
		    else {
                muduo::string message;
                if (pSess->stage_ != SC_PASSED || !pSess->decrypt(pC1, &message, len)) {
                    LOG_WARN << "Stage / decryption error";
                    conn->shutdown();
                    break;
                }
                buf->retrieve(len + 4);

                switch (cmd){
                case CAA_KALIVE:
                    pSess->sendPacket(CAS_KALIVE, NULL, 0);
                    break;
                case CAA_DEVSTA:
                    DevStatus(conn, pSess, message);
                    break;
                case CAA_DATA:
                    NewData(conn, pSess, message);
                    break;
                default:
                    LOG_WARN << "Unknown command type: " << cmd;
                    //conn->shutdown();
                    break;
                }
		    }
		}
		else break;	// Haven't got enough bytes
    }
}


void AgtServer::DevStatus(const TcpConnectionPtr& conn,
                          Session * pSess,
                          const muduo::string& message)
{
    uint16_t tLen = 0;
    std::set<uint64_t> ts;
    uint64_t dId;

    while (tLen < message.length()) {
        DevInfoHeader *pInfo = (DevInfoHeader *)(message.data() + tLen);
        tLen += muduo::net::sockets::networkToHost16(pInfo->len_);
        if (tLen > message.length()) break;
        dId = Get64FromDevInfo(pInfo);
        switch (pInfo->type_) {
        case DP_BASIC:
            {
                DevBasic *pDev = (DevBasic *)pInfo->con_;
                // TODO: Query and back?
            }
            break;
        case DP_LOST:
            // Record & broadcast device lost
            // Write device lost
            if (ts.count(dId) == 0) {
                ts.insert(dId);
                pSess->sendDevAck(pInfo);
            }
            break;
        case DP_ERRHIS:
            //
            if (ts.count(dId) == 0) {
                ts.insert(dId);
                pSess->sendDevAck(pInfo);
            }
            break;
        case DP_ERRHISF:

            if (ts.count(dId) == 0) {
                ts.insert(dId);
                pSess->sendDevAck(pInfo);
            }
            break;
        default: break;
        }
    }
}

void AgtServer::NewData(const TcpConnectionPtr& conn,
                          Session * pSess,
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









