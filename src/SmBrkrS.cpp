//============================================================================
// Name        : SmartBreakerServer.cpp
// Author      : Bingyu
// Version     :
// Copyright   : ZEin 2017
// Description :
//============================================================================

#include <muduo/base/Mutex.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/inspect/Inspector.h>

#include <boost/bind.hpp>
#include <boost/random.hpp>
#include <boost/random/seed_seq.hpp>

#include <string.h>
#include <unistd.h>

#include <iostream>


using namespace muduo;
using namespace muduo::net;

//using namespace std;

#include <ctime>

#include "gconf.h"
#include "commons.h"

#include "redisWrapper.h"
#include "servers.h"




namespace muduo
{
namespace inspect
{
int stringPrintf(string* out, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
}
}

using muduo::inspect::stringPrintf;

boost::mt19937 gRng;

string GetOverview(HttpRequest::Method, const Inspector::ArgList&,
                   AgtServer *pAgt, HookServer *pHook)
{
	string result;

	result.reserve(1024);
	stringPrintf(&result, "Current active links:\n\n");
	stringPrintf(&result, "Agents: %5d / %d\n", pAgt->getNumConn(), gConf.agtCmax_);
	stringPrintf(&result, "Hooks:  %5d / %d\n", pHook->getNumConn(), gConf.hookCmax_);

    return result;
}

void SBSMain(void)
{
    unsigned tt[2] = { static_cast<unsigned int>(std::time(0)), 2323ul };

    boost::random::seed_seq ss(tt);
	gRng.seed(ss);

	EventLoop loop;
	EventLoopThread tIns, tQuery;
	Inspector ins(tIns.startLoop(), InetAddress(gConf.monPort_), "Inspector");

	hiredis::redisQuery qRedis(&loop, InetAddress(gConf.rds1Addr_, gConf.rds1Port_));
	qRedis.connect();

	hiredis::redisStore sRedis(tQuery.startLoop(), InetAddress(gConf.rds1Addr_, gConf.rds1Port_));
    sRedis.connect();

    HookServer hookSvr(&loop, InetAddress(static_cast<uint16_t>(gConf.hookPort_)),
                       gConf.hookCmax_, "TCPHook");
    hookSvr.start();

	// Start Agent Listener
    AgtServer agtServer(&loop, InetAddress(static_cast<uint16_t>(gConf.agtPort_)),
                      gConf.agtCmax_, "AgentServer", &sRedis, &qRedis, &hookSvr);
    agtServer.start();

	// Add inspector entrance

	ins.add("smbrk", "links",
            boost::bind(GetOverview, _1, _2, &agtServer, &hookSvr),
            "Active links overview");

	loop.loop();
}
