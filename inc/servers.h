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

#include <set>


class AgtServer
{
public:
	AgtServer(EventLoop* loop, const InetAddress& listenAddr,
              int maxConn, const muduo::string& svrName)
            : loop_(loop), kMaxConn_(maxConn), numConnected_(0),
			  server_(loop, listenAddr, svrName)
//			  codec_(boost::bind(&AgtServer::onBlockMessage, this, _1, _2, _3))
	{
		server_.setConnectionCallback(boost::bind(&AgtServer::onConnection, this, _1));
		server_.setMessageCallback(
//					boost::bind(&MLengthHeaderCodec::onMessagewC, this, _1, _2, _3));
                    boost::bind(&AgtServer::onMessage, this, _1, _2, _3));

	}

	void start()
	{
		server_.start();
	}

    virtual ~AgtServer() {}

protected:

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

//	MLengthHeaderCodec codec_;

	MutexLock 		mutex_;

private:

};


#endif // AGTSERVER_H
