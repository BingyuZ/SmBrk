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

using namespace std;

#include <ctime>

#include "gconf.h"
#include "commons.h"

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


void SBSMain(void)
{
    unsigned tt[2] = { static_cast<unsigned int>(std::time(0)), 2323ul };

    boost::random::seed_seq ss(tt);
	gRng.seed(ss);

	EventLoop loop;
	EventLoopThread t;
	Inspector ins(t.startLoop(), InetAddress(gConf.monPort_), "Inspector");

	// Start Listener
    AgtServer aServer(&loop, InetAddress(static_cast<uint16_t>(gConf.agtPort_)),
                      gConf.agtCmax_, "AgentServer");
    aServer.start();

	// Add inspector entrance

	//ins.add()

	loop.loop();
}
