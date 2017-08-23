#ifndef AGTSERVER_H
#define AGTSERVER_H

#include <stdint.h>

#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
//#include <muduo/net/TimerId.h>
#include <muduo/net/inspect/Inspector.h>

#include "zecodec.h"
#include "devInfo.h"

#include <set>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>


#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const boost::shared_ptr<T>& x)
{
  return boost::hash_value(x.get());
}
}
#endif


namespace hiredis
{
class redisStore;
class redisQuery;
}

class HookServer;

class AgtServer
{
public:
	AgtServer(EventLoop* loop, const InetAddress& listenAddr,
              int maxConn, const muduo::string& svrName,
              hiredis::redisStore *pSRedis, hiredis::redisQuery *pQRedis,
              HookServer *pHook,
              int idleSeconds
              )
            : loop_(loop), kMaxConn_(maxConn), numConnected_(0),
			  server_(loop, listenAddr, svrName),
			  pSRedis_(pSRedis), pQRedis_(pQRedis), pHook_(pHook),
			  connectionBuckets_(idleSeconds)
//			  codec_(boost::bind(&AgtServer::onBlockMessage, this, _1, _2, _3))
	{
		server_.setConnectionCallback(boost::bind(&AgtServer::onConnection, this, _1));
		server_.setMessageCallback(
//                    boost::bind(&MLengthHeaderCodec::onMessagewC, &codec_, _1, _2, _3));
                  boost::bind(&AgtServer::onMessage, this, _1, _2, _3));
        loop->runEvery(5.0, boost::bind(&AgtServer::onTimer, this));
        connectionBuckets_.resize(idleSeconds);
	}

	void start()
	{
		server_.start();
	}

    virtual ~AgtServer() {}

    unsigned getNumConn(void) { return numConnected_; }

    typedef boost::shared_ptr<Session> SessionPtr;

protected:
//    bool CheckCRC(const uint8_t*, uint32_t len);

    int DevStatus(const TcpConnectionPtr&, Session *, const muduo::string&);
    int NewData(const TcpConnectionPtr&, Session *, const muduo::string&);
//    void SaveHistory(const TcpConnectionPtr&, Session *, const muduo::string&);
//    void DevLost(const TcpConnectionPtr&, Session *, const muduo::string&);

    void onConnection(const TcpConnectionPtr& conn);

    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp);

	void onBlockMessage(const TcpConnectionPtr& conn,
						const muduo::string& message,
						Timestamp);

	typedef std::set<TcpConnectionPtr> ConnectionList;
	ConnectionList	connections_;

	EventLoop* 		loop_;
	int				kMaxConn_;
	int				numConnected_;

    TcpServer 		server_;
    hiredis::redisStore *pSRedis_;
    hiredis::redisQuery *pQRedis_;
    HookServer      *pHook_;

//	MLengthHeaderCodec codec_;
	MutexLock 		mutex_;

	// Idle treatment
    void onTimer()
    {
        connectionBuckets_.push_back(Bucket());
    }

    typedef boost::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;
    struct Entry : public muduo::copyable
    {
        explicit Entry(const WeakTcpConnectionPtr& weakConn) : weakConn_(weakConn) {}
        ~Entry()
        {
            muduo::net::TcpConnectionPtr conn = weakConn_.lock();
            if (conn) conn->shutdown();
        }
        WeakTcpConnectionPtr weakConn_;
    };
    typedef boost::shared_ptr<Entry> EntryPtr;
    typedef boost::weak_ptr<Entry> WeakEntryPtr;
    typedef boost::unordered_set<EntryPtr> Bucket;
    typedef boost::circular_buffer<Bucket> WeakConnectionList;
    WeakConnectionList connectionBuckets_;

private:
    const static int32_t kMinLength = 0x10;
	const static size_t kHeaderLen = sizeof(int32_t);
};


class HookServer
{
public:
	HookServer(EventLoop* loop, const InetAddress& listenAddr,
              int maxConn, const muduo::string& svrName)
            : loop_(loop), kMaxConn_(maxConn), numConnected_(0),
			  server_(loop, listenAddr, svrName)
	{
		server_.setConnectionCallback(boost::bind(&HookServer::onConnection, this, _1));
		server_.setMessageCallback(
//                    boost::bind(&MLengthHeaderCodec::onMessagewC, &codec_, _1, _2, _3));
                  boost::bind(&HookServer::onMessage, this, _1, _2, _3));
	}

	void start()
	{
		server_.start();
	}

    virtual ~HookServer() {}

    void broadcast();   // Test
    void broadcast(const uint8_t *dId, DevData *pDev, unsigned len);

    unsigned getNumConn(void) { return numConnected_; }

protected:
    void onConnection(const TcpConnectionPtr& conn);

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp);

	typedef std::set<TcpConnectionPtr> ConnectionList;
	ConnectionList	connections_;

	EventLoop* 		loop_;
	int				kMaxConn_;
	int				numConnected_;

    TcpServer 		server_;
   	MutexLock 		mutex_;
};

class CmdServer
{
public:
    CmdServer(EventLoop* loop, const InetAddress& listenAddr,
              int maxConn, const muduo::string& svrName)
            : loop_(loop), kMaxConn_(maxConn), numConnected_(0),
			  server_(loop, listenAddr, svrName)
	{
		server_.setConnectionCallback(boost::bind(&CmdServer::onConnection, this, _1));
		server_.setMessageCallback(
//                    boost::bind(&MLengthHeaderCodec::onMessagewC, &codec_, _1, _2, _3));
                  boost::bind(&CmdServer::onMessage, this, _1, _2, _3));
	}

	void start()
	{
		server_.start();
	}

    virtual ~CmdServer() {}

    unsigned getNumConn(void) { return numConnected_; }
    void sendPacket(const TcpConnectionPtr& conn, CommandAgt type,
                    const void *pBuf, uint32_t length);

protected:
    void onConnection(const TcpConnectionPtr& conn);

	bool decrypt(const uint8_t *src, muduo::string *msg, int length);

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp);

    int command(const TcpConnectionPtr& conn, const muduo::string& message);

    int findAndSend(uint64_t dId, const CmdReqs* pCmd);

	typedef std::set<TcpConnectionPtr> ConnectionList;
	ConnectionList	connections_;

	EventLoop* 		loop_;
	int				kMaxConn_;
	int				numConnected_;

    TcpServer 		server_;
   	MutexLock 		mutex_;

    const static int32_t kSpec = static_cast<int32_t>(('Z'<<24) | ('E'<<16));
    const static int     kMinLength = 8;
};

#endif // AGTSERVER_H
