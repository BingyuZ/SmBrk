#include "servers.h"
#include "commons.h"
#include "redisWrapper.h"

#include <boost/random.hpp>

#include <memory.h>
#include <sys/time.h>

#include "colors.h"

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

#define MAXCROUND   90

DevMap gDevMap;

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
        seed = seed * 6364136223846793005ull + 1;
        return seed >> 32;
    }
}

static const char hexStr[] = "0123456789abcdef";


char *GetIdfrom64(uint64_t t)
{
    static char tId[20];
    uint8_t k;
    char *tt = tId;

    for (int i=0; i<8; i++) {
        k = (t >> 56);
        t <<= 8;
        *tt++ = hexStr[k/16];
        *tt++ = hexStr[k%16];
    }
    *tt = 0;
    return tId;
}

void PrintId(const uint8_t *p, char *tt)
{
    for (int i=0; i<6; i++) {
        *tt++ = hexStr[p[i] / 16];
        *tt++ = hexStr[p[i] % 16];
    }
    *tt = 0;
}

uint64_t Get64FromDevInfo(DevInfoHeader *pInfo)
{
    uint64_t r = 0;
    //= pInfo->devType_;
    for (int i=0; i<6; i++)
        r = (r << 8) | pInfo->dID_[i];
    return r;
}

uint32_t Char6ToUInt32(const uint8_t *s)
{
	uint32_t v;
	sscanf((const char *)s, "%6x", &v);
	return v;
}

bool UInt32ToChar6(uint32_t v, char *s)
{
	v = v & 0xffffffUL;
	snprintf(s, 6, "%06x", v);
	return true;
}



static int GetGPRSInfo(GPRSInfo *pInfo, const muduo::string&msg)
{
    PacketHeader *pHeader = (PacketHeader *)msg.data();
    int type = pHeader->body_[4]*256 + pHeader->body_[5];
    unsigned length = pHeader->body_[6]*256 + pHeader->body_[7];

    LOG_DEBUG << "MsgLength: " << msg.length() << " Len:" << length;
    if (msg.length() < length + 0x10) return -1;
    if (type != 1) return -2;

    memcpy(pInfo, pHeader->body_+8, sizeof(GPRSInfo));
    return type;
}

bool Session::addDevice(const uint8_t *dId, uint8_t modId)
{
    // First, search whether is same id
    int i;
    for (i=0; i<MAXDEV; i++) {
        if (!memcmp(dId, devId_[i].dId_, 6)) {
            devId_[i].modId_ = modId;
            devId_[i].sta_ = 1;     // Active
            return true;
        }
    }
    for (i=0; i<MAXDEV; i++) {
        if (devId_[i].sta_ == 0) {
            memcpy(devId_[i].dId_, dId, 6);
            devId_[i].modId_ = modId;
            devId_[i].sta_ = 1;
            return true;
        }
    }
    return false;
}

bool Session::delDevice(const uint8_t *dId)
{
    int i;
    for (i=0; i<MAXDEV; i++) {
        if (!memcmp(dId, devId_[i].dId_, 6)) {
            devId_[i].sta_ = 0;    // Active
            return true;
        }
    }
    char s[20];
    PrintId(dId, s);
    LOG_DEBUG << "Cannot find " << s << " when deleting device";
    return false;
}

#define MASK6 0xffffffffffffull
bool Session::addDevice(uint64_t dId, uint8_t modId)
{
    int i;
    for (i=0; i<MAXDEV; i++) {
        if (dId == (devId_[i].uId_ & MASK6))
            goto found;
    }
    for (i=0; i<MAXDEV; i++) {
        if (devId_[i].sta_ == 0) {
            devId_[i].uId_ = dId;
            goto found;
        }
    }
    return false;

found:
    devId_[i].modId_ = modId;
    devId_[i].sta_ = 1;     // Active
    LOG_DEBUG << "Add dev " << modId << " AT " << i;

    return true;
}

bool Session::delDevice(uint64_t dId)
{
    for (int i=0; i<MAXDEV; i++) {
        if (dId == (devId_[i].uId_ & MASK6)) {
            devId_[i].sta_ = 0;    // Active
            LOG_DEBUG << "Remove dev " << devId_[i].modId_ << " AT " << i;
            //DelMapDev(dId, this);
            return true;
        }
    }
    char s[20];
    sprintf(s, "%lx", dId);
    LOG_DEBUG << "Cannot find " << s << " when deleting device";
    return false;
}

#if 0
    char s[100];
    devId_[0].uId_ = dId;
    devId_[0].modId_ = 2;
    devId_[0].sta_ = 1;
    sprintf(s, "Before:%lx  After:%lx", dId, devId_[0].uId_);
    LOG_DEBUG << "TESTID: " << s;
#endif

void Session::sendDevAck(DevInfoHeader *pInfo)
{
    DevInfoHeader head;

    head.type_ = DP_ACK;
    memcpy(head.dID_, pInfo->dID_, 6);
    head.len_ = sizeof(head);

    sendPacket(CAS_DEVANS, &head, head.len_);
}

void Session::sendDevHis(const uint8_t *dId, const DevErrHis *pHist)
{
    uint8_t buf[sizeof(DevInfoHeader) + sizeof(DevErrHis)+4];
    DevInfoHeader *pHead = (DevInfoHeader *)buf;

    pHead->type_ = DP_LASTHIS;
    memcpy(pHead->dID_, dId, 6);
    pHead->len_ = sizeof(DevInfoHeader) + sizeof(DevErrHis);

    memcpy(pHead->con_, pHist, sizeof(DevErrHis));

    sendPacket(CAS_DEVANS, pHead, pHead->len_+4);
}

void Session::sendDevHisN(const uint8_t *dId)
{
    DevInfoHeader head;

    head.type_ = DP_LASTNAK;
    memcpy(head.dID_, dId, 6);
    head.len_ = sizeof(head);

    sendPacket(CAS_DEVANS, &head, head.len_);
}

void Session::sendDataAck(DevInfoHeader *pInfo)
{
    DevInfoHeader head;

    head.type_ = DP_DATAACK;
    //head.devType_ = pInfo->devType_;
    memcpy(head.dID_, pInfo->dID_, 6);
    head.len_ = sizeof(head);

    sendPacket(CAS_DACK, &head, head.len_);
}

void Session::sendLogRes(const uint8_t *pC1)
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

        EntryPtr entry(new Entry(conn));
        connectionBuckets_.back().insert(entry);

        WeakEntryPtr weakEntry(entry);

//1        Session sess(conn, weakEntry);
        SessionPtr pSess(new Session(conn, weakEntry));
//1        sess.sendWelcome();
        pSess->sendWelcome();

        MutexLockGuard lock(mutex_);

        ++numConnected_;
//1        conn->setContext(sess);
        conn->setContext(pSess);

        sessions_.insert(pSess);
//2        connections_.insert(conn);
	}
	else
    {
//1        Session* pSess = boost::any_cast<Session>(conn->getMutableContext());
//1        pSess->ResetConn();

        SessionPtr pSess(boost::any_cast<SessionPtr>(conn->getContext()));

        MutexLockGuard lock(mutex_);
        pSess->ResetConn();

        for (int i=0; i<MAXDEV; i++){
            if (pSess->devId_[i].sta_) {
                pSess->devId_[i].sta_ = 0;
                DevMap::iterator it = gDevMap.find(pSess->devId_[i].uId_ & MASK6);
                if (it != gDevMap.end()) {
                    if (it->second == pSess) {
                        LOG_DEBUG << "Dev@Map found and deleted";
                        gDevMap.erase(it);
                    }
                }
            }
        }

        conn->setContext(NULL);
        //conn->setContext(SessionPtr());

		--numConnected_;
		sessions_.erase(pSess);
//2		connections_.erase(conn);
	}
}

//  assert(!conn->getContext().empty());
//  Node* node = boost::any_cast<Node>(conn->getMutableContext());
//  node->lastReceiveTime = time;

// Returns false on error
bool CheckCRC(const uint8_t *c, uint32_t len)
{
    // TODO: Check CRC
    return true;
}

void AgtServer::onTimer(void)
{
    connectionBuckets_.push_back(Bucket());

}

void AgtServer::onMessage(  const muduo::net::TcpConnectionPtr& conn,
	  	  	  				muduo::net::Buffer* buf,
	  	  	  				muduo::Timestamp t)
{
    while (buf->readableBytes() >= kMinLength + 4) // kMinLength == 16  exclude CRC
    {
		const uint8_t* pC1 = (const uint8_t *) buf->peek();

        if (*pC1 != 'Z' && *(pC1+1) != 'E') {
			LOG_ERROR << "Wrong packet head " << *pC1 << " " << *(pC1+1);
			goto errorQuit;
		}

        pC1 += 2;
		uint32_t len = *pC1 * 256 + *(pC1+1);

		if (len > 32768 || len < kMinLength - 4)   // Length = 12+
		{
			LOG_WARN << "Invalid packet length " << len;
			goto errorQuit;
		}
		else if (buf->readableBytes() >= len + 8)   // Header + CRC
		{
		    uint8_t cmd;
		    if (!CheckCRC(pC1-2, len)) {
                LOG_WARN << "CRC Error";
                goto errorQuit;
		    }
		    pC1 += 2;
            buf->retrieve(kHeaderLen);  // 4
		    LOG_DEBUG << ZEC_CYAN << "Packet received, len=" << len << ", Type: " << 0L+*pC1 << ZEC_RESET;

            assert(!conn->getContext().empty());
//1            Session *pSess  = boost::any_cast<Session>(conn->getMutableContext());

            SessionPtr pSess(boost::any_cast<SessionPtr>(conn->getContext()));

            WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(pSess->GetPW()));
            EntryPtr entry(weakEntry.lock());
            if (entry) connectionBuckets_.back().insert(entry);

            cmd = *pC1;
		    if (cmd == CAA_LOGANS) {   // Decryption not needed for LOGANS.
		        //SessionPtr *pSess = boost::any_cast<SessionPtr>(conn->getMutableContext());
                muduo::string message(buf->peek(), len);
                if ((len != 16 && len <20) || pSess->stage_ != SC_CHSENT
                     || !pSess->CheckPass(message)) {
                    LOG_WARN << "Stage / password error";
                    goto errorQuit;
                }
                uint32_t aid = muduo::net::sockets::networkToHost32(*(uint32_t *)(pC1+12));
                buf->retrieve(len + 4);

                if (len == 16) {
                    LOG_INFO << "Agent " << aid << " Logged in";
                    pSRedis_->agentLogin(aid);
                }
                else {
                    GPRSInfo gprs;
                    int ret;
                    if ((ret = GetGPRSInfo(&gprs, message)) > 0) {
                        LOG_INFO << "Agent " << aid << " Logged in, ASU=" << gprs.asu_;
                        pSRedis_->agentLogin(aid, &gprs);
                    }
                    else {
                        LOG_WARN << "GPRSInfo error:" << ret;
                        goto errorQuit;
                    }
                }
                pSess->sendLogRes(pC1);
		    }   // CAA_LOGANS
		    else {
                muduo::string message;
                if (pSess->stage_ != SC_PASSED || !pSess->decrypt(pC1, &message, len)) {
                    LOG_WARN << "Stage / decryption error";
                    goto errorQuit;
                }
                buf->retrieve(len + 4);

                switch (cmd){
                case CAA_KALIVE:
                    pSess->sendPacket(CAS_KALIVE, NULL, 0);
                    break;
                case CAA_DEVSTA:
                    if (DevStatus(conn, pSess, message) < 0)
                        goto errorQuit;
                    break;
                case CAA_DATA:
                    if (NewData(conn, pSess, message) < 0)
                        goto errorQuit;
                    break;
                case CAA_CMDRESP:
					if (CmdResp(conn, pSess, message) < 0)
						goto errorQuit;
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
    return;
errorQuit:
    conn->shutdown(); // FIXME: disable reading
}


int AgtServer::DevStatus(const TcpConnectionPtr& conn,
                         const SessionPtr& pSess,
                         const muduo::string& message)
{
    uint16_t pos = 0;
    std::set<uint64_t> ts;
    uint64_t dId;

    while (pos + 4 < (uint16_t)message.length()) {
        DevInfoHeader *pInfo = (DevInfoHeader *)(message.data() + pos);
        char Sid[20];

        PrintId(pInfo->dID_, Sid);

        LOG_DEBUG << ZEC_GREEN << "Pos:" << pos << " Type:" << pInfo->type_
                  << " Len:" << pInfo->len_ << " ID:" << Sid << ZEC_RESET;
        if (pInfo->len_ < 8) {
            LOG_DEBUG << "Invalid dev status length: " << pInfo->len_;
            return -1;
        }
        pos += pInfo->len_;
        if (pos > message.length()) {
            LOG_DEBUG << "Invalid dev status pos: " << pos;
            return -2;
        }
        dId = Get64FromDevInfo(pInfo);

        switch (pInfo->type_) {
        case DP_BASIC:
            {
                DevBasic *pDev = (DevBasic *)pInfo->con_;
                if (!pSess->addDevice(dId, pDev->modId_)) {
                    // Too many device?
                    LOG_DEBUG << "Add device error:";
                }
                else {
                    gDevMap[dId] = pSess;

                    pSRedis_->devProp(pInfo->dID_, pDev, pInfo->len_-sizeof(DevInfoHeader));

                    // Write device login
                    pSRedis_->devLogin(pInfo->dID_, pSess->agentId_, pDev->modId_);
                    // TODO: broadcast device login

                    pQRedis_->checkLastErr(pSess, pInfo->dID_);
                }
            }
            break;
        case DP_LOST:
            if (pSess->delDevice(dId)) {

                DevMap::iterator it = gDevMap.find(dId);
                if (it != gDevMap.end()) {
                    if (it->second == pSess) {
                        LOG_DEBUG << "Dev@Map found and deleted";
                        gDevMap.erase(it);
                    }
                    else {
                        LOG_DEBUG << "Dev is different in map";
                    }
                }
                else {
                    LOG_DEBUG << "Dev not found in map";
                }

                pSRedis_->devLogout(pInfo->dID_);

                // TODO: broadcast device LOST
                if (ts.count(dId) == 0) {
                    ts.insert(dId);
                    pSess->sendDevAck(pInfo);
                }
            }
            break;
        case DP_ERRHIS:
            {
                DevErrHis *pErr = (DevErrHis *)pInfo->con_;
                pSRedis_->errHist(pInfo->dID_, pErr);
            }
            if (ts.count(dId) == 0) {
                ts.insert(dId);
                pSess->sendDevAck(pInfo);
            }
            break;
        case DP_ERRHISF:
            {
                DevErrHisF *pErr = (DevErrHisF *)pInfo->con_;
                pSRedis_->errHistF(pInfo->dID_, pErr);
            }
            if (ts.count(dId) == 0) {
                ts.insert(dId);
                pSess->sendDevAck(pInfo);
            }
            break;
        default:
            LOG_WARN << "Unknown Dev subcommand : " << pInfo->type_;
            // TODO: Send Error
            break;
        }
    }
    return 0;
}

int AgtServer::CmdResp(const TcpConnectionPtr& conn,
                       const SessionPtr& pSess,
                       const muduo::string& message)
{
	int ret = -1;
	CmdWrapper *wHead = (CmdWrapper *)message.data();
	if (wHead->len_ <= (uint16_t)message.length() &&
		wHead->len_ >= 24)
	{
		CmdReply *pReply = (CmdReply *)wHead->content_;
		if (pReply->len_ <= (uint16_t)message.length() - 8 &&
			pReply->len_ >= 16)
		{
			uint32_t msgID_ = Char6ToUInt32(wHead->msgID_);

		}
	}
quit:
	return ret;
}

int AgtServer::NewData(const TcpConnectionPtr& conn,
                       const SessionPtr& pSess,
                       const muduo::string& message)
{
    uint16_t pos = 0;

    while (pos + 4 < (uint16_t)message.length()) {
        char Sid[20];
        DevInfoHeader *pInfo = (DevInfoHeader *)(message.data() + pos);
        PrintId(pInfo->dID_, Sid);

        LOG_DEBUG << ZEC_GREEN << "Pos:" << pos << " Type:" << pInfo->type_
                  << " Len:" << pInfo->len_ << " ID:" << Sid << ZEC_RESET;
        pos += pInfo->len_;
        if (pInfo->len_ < 4) {
            LOG_DEBUG << "Invalid data length: " << pInfo->len_;
            return -1;
        }
        if (pos > message.length()) {
            LOG_DEBUG << "Invalid data pos: " << pos;
            return -1;
        }

        // TODO: special treatment should be done for
        //  DP_DATAUPF with duration_ == 0
        if (pInfo->type_ == DP_DATAUPF) {
            DevErrHisF *pErr = (DevErrHisF *)pInfo->con_;
            pSRedis_->errHistF(pInfo->dID_, pErr);
        }
        #if 0
        else if (pInfo->type_ == DP_DATAUPG) {
            DevErrHisF *pErr = (DevErrHisF *)pInfo->con_;
            pSRedis_->errRHist(pInfo->dID_, pErr);
        }
        #endif  // Disable UPG
        if (pInfo->type_ == DP_DATAUP || pInfo->type_ == DP_DATAUPF
            || pInfo->type_ == DP_DATAUPG)
        {
            DevData *pDev;
            if (pInfo->type_ == DP_DATAUP) pDev = (DevData *)pInfo->con_;
            else pDev = (DevData *)&pInfo->con_[sizeof(DevErrHisF)];

            int len = pInfo->len_ - sizeof(DevInfoHeader) - 2;
            if (pInfo->type_ != DP_DATAUP)
                len -= sizeof(DevErrHisF);

            pSRedis_->dataRpt(pInfo->dID_, pDev, len);
            pSess->sendDataAck(pInfo);
            pHook_->broadcast(pInfo->dID_, pDev, len+2);
        }
        else {
            LOG_WARN << "Unknown Dev subcommand : " << pInfo->type_;
            // TODO: Send Error
        }
    }
    return 0;
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

void HookServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << "Hook: " << conn->localAddress().toIpPort() << " -> "
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
        conn->setTcpNoDelay(false);

        MutexLockGuard lock(mutex_);
        ++numConnected_;
        connections_.insert(conn);
	}
	else
    {
        MutexLockGuard lock(mutex_);
		--numConnected_;
		connections_.erase(conn);
	}
}

void HookServer::onMessage( const muduo::net::TcpConnectionPtr& conn,
	  	  	  				muduo::net::Buffer* buf,
	  	  	  				muduo::Timestamp t)
{
    LOG_DEBUG << "Hook: " << buf->readableBytes() << " are waiting.";
	buf->retrieveAll();
}

void HookServer::broadcast()
{
    LOG_INFO << "size = " << connections_.size();

    for (ConnectionList::iterator it = connections_.begin();
        it != connections_.end(); ++it)
    {
        if ((*it)->connected()) (*it)->send("Hello");
    }
}


void HookServer::broadcast(const uint8_t *dId, DevData *pDev, unsigned len)
{
    HookHeader header;

    header.len_[0] = (len + 8)/256;
    header.len_[1] = (len + 8)%256;
    memcpy(header.dId_, dId, 6);

//    muduo::net::Buffer buf;
//    buf.append(&header, sizeof(header));
//    buf.append(pDev, len);

    for (ConnectionList::iterator it = connections_.begin();
        it != connections_.end(); ++it)
    {
        if ((*it)->connected()) {
            (*it)->send(&header, sizeof(header));
            (*it)->send(pDev, len);
        }
    }
}


// Delete all outdated pairs
void CmdServer::clearConn(void)
{
    time_t margin;
    timeval tv;
    gettimeofday(&tv, NULL);
    margin = tv.tv_sec - MAXCROUND;

    MutexLockGuard lock(mMutex_);

    ItemMap::iterator it = map_.begin();
    while (it != map_.end()) {
        if (it->second->cStamp_ < margin) {
            map_.erase(it++);   // The compiler will backup it first
        }
        else it++;
    }
}

bool CmdServer::sendReply(uint32_t cmdIdx, const CmdReply *pReply, uint32_t len)
{
    bool result = false;

    ItemMap::iterator it = map_.find(cmdIdx);
    if (it != map_.end()) {
        muduo::net::TcpConnectionPtr conn = it->second->weakConn_.lock();
        if (conn) {
            // TODO:
            sendPacket(conn, CCS_CMDRESP, pReply, len);
            result = true;

            MutexLockGuard lock(mMutex_);
            map_.erase(it);
        }
		else {
			LOG_DEBUG << "Command agent disconnected";
		}
    }
    return result;
}

bool CmdServer::addConn(const TcpConnectionPtr& conn, uint32_t *pCmdIdx)
{
    uint32_t idx;

    do {
        idx = GetMyRand() & 0xfffffful;
    } while (map_.count(idx) > 0);

    WeakTcpConnectionPtr weakConn(conn);
    ConnStPtr connPtr(new ConnStamp(weakConn));
    map_[idx] = connPtr;
    *pCmdIdx = idx;
    return true;
}

void CmdServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << "Cmd: " << conn->localAddress().toIpPort() << " -> "
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
        conn->setTcpNoDelay(false);

        MutexLockGuard lock(mutex_);
        ++numConnected_;
        connections_.insert(conn);
	}
	else
    {
        MutexLockGuard lock(mutex_);
		--numConnected_;
		connections_.erase(conn);
	}
}

bool CmdServer::decrypt(const uint8_t *src, muduo::string *msg, int length)
{
    msg->reserve(length);
    src += 4;
    if (length < 5) return true;

    for (int i=0; i<((length+3)&0xfff8); i+=8) {
        msg->append((const char *)src, 8);
        src += 8;
    }
    LOG_DEBUG << msg->length() << " bytes decrypted";
    return true;
}

void CmdServer::sendPacket(const TcpConnectionPtr& conn, CommandAgt type,
                          const void *pBuf, uint32_t length)
{
    const char *ptr = (const char *)pBuf;

	assert(length % 8 == 0);
	assert(length < 65501);

	PacketHeader header;
    header.cmd_ = type;
	header.cmdA_ = 0;
	header.ver_ = ZE_VER;
    header.rev_ = 0;

	uint32_t first = kSpec + 4 + length;

    muduo::net::Buffer buf;
    buf.append(&header, 4);
	buf.prependInt32(first);

	length = (length+7) & 0xfff8;

    for (uint32_t i=0; i<length; i+=8) {
        buf.append(ptr, 8);
        ptr += 8;
    }

    uint32_t crc = 0xbbbbbbbb;   // TODO: Real CRC
    buf.appendInt32(crc);

    conn->send(&buf);

}

// -1 on not found
int CmdServer::findAndSend(const TcpConnectionPtr& conn, uint64_t dId, const CmdReqs* pCmd)
{
    DevMap::iterator itDM = gDevMap.find(dId);
    if (itDM == gDevMap.end()) return -1;

	uint32_t reqID;
    // We find the session now!
    // register the operation
	addConn(conn, &reqID);
	// The request will be sent to the agent and not kept any more
	// length in pCmd->len_
	// TODO:
//	itDM->second->Send
}


int CmdServer::command(const TcpConnectionPtr& conn, const muduo::string& message)
{
    uint16_t pos = 0;
    uint64_t dId;
    int ret;

    while (pos + 6 < (uint16_t)message.length()) {
        CmdReqs *pReq = (CmdReqs *)(message.data() + pos);
        char Sid[20];
        CmdReply reply;

        PrintId(pReq->dID_, Sid);
        LOG_DEBUG << ZEC_RED << "Pos:" << pos << " Type:" << pReq->type_
                  << " Len:" << pReq->len_ << " ID:" << Sid << ZEC_RESET;

        if (pReq->len_ < 12) {
            LOG_DEBUG << "Invalid dev status length: " << pReq->len_;
            return -1;
        }
        pos += pReq->len_;
        if (pos > message.length()) {
            LOG_DEBUG << "Invalid dev status pos: " << pos;
            return -2;
        }

        // TODO:
        dId = Get64FromDevInfo((DevInfoHeader *)pReq);
        ret = findAndSend(conn, dId, pReq);
        if (ret < 0) {
            LOG_DEBUG << "Device " << Sid << "not found:" << ret;
            memcpy(&reply, pReq, sizeof(CmdReqs));
            memset(&reply.result_, 0, 4);
            reply.len_ = sizeof(CmdReqs);
            reply.type_ = CMD_DEVLOST;
            sendPacket(conn, CCS_CMDRESP, &reply, sizeof(CmdReply));
        }
    }
    return 0;
}


void CmdServer::onMessage( const muduo::net::TcpConnectionPtr& conn,
	  	  	  				muduo::net::Buffer* buf,
	  	  	  				muduo::Timestamp t)
{
    while (buf->readableBytes() >= kMinLength + 4) // kMinLength == 16  exclude CRC
    {
		const uint8_t* pC1 = (const uint8_t *) buf->peek();

        if (*pC1 != 'Z' && *(pC1+1) != 'E') {
			LOG_ERROR << "Wrong packet head " << *pC1 << " " << *(pC1+1);
			goto errorQuit;
		}

        pC1 += 2;
		uint32_t len = *pC1 * 256 + *(pC1+1);

		if (len > 32768 || len < kMinLength - 4)   // Length = 8+
		{
			LOG_WARN << "Invalid packet length " << len;
			goto errorQuit;
		}
		else if (buf->readableBytes() >= len + 8)   // Header + CRC
		{
		    uint8_t cmd;
		    if (!CheckCRC(pC1-2, len)) {
                LOG_WARN << "CRC Error";
                goto errorQuit;
		    }
		    pC1 += 2;
            buf->retrieve(4);  // 4
		    LOG_DEBUG << ZEC_CYAN << "Packet received, len=" << len << ", Type: " << 0L+*pC1 << ZEC_RESET;

            //assert(!conn->getContext().empty());

            cmd = *pC1;

		    {
                muduo::string message;
                if (!decrypt(pC1, &message, len)) {
                    LOG_WARN << "Decryption error";
                    goto errorQuit;
                }
                buf->retrieve(len + 4);

                switch (cmd){
                case CAA_KALIVE:
                    sendPacket(conn, CAS_KALIVE, NULL, 0);
                    break;
                case CCA_COMMAND:
                    if (command(conn, message) < 0) goto errorQuit;
                    break;
                default:
                    LOG_WARN << "Unknown command type: " << cmd;
                    //conn->shutdown();
                    break;
                }
		    }
		}
		else break;
    }
    return;
errorQuit:
    conn->shutdown(); // FIXME: disable reading
}




//#endif // 0









