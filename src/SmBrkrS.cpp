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

#include <string.h>
#include <unistd.h>

#include <iostream>


using namespace muduo;
using namespace muduo::net;

using namespace std;
//using namespace zeinmod;

#include <ctime>

#include "gconf.h"
#include "commons.h"

#include "ConnMan.h"
#include "servers.h"

boost::mt19937 gRng(2323u);


namespace muduo
{
namespace inspect
{
int stringPrintf(string* out, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
}
}

using muduo::inspect::stringPrintf;







void SBSMain(void)
{
	gRng.seed(static_cast<unsigned int>(std::time(0)));

	EventLoop loop;
	EventLoopThread t;
	Inspector ins(t.startLoop(), InetAddress(gConf.monPort_), "Inspector");

	// Start Listener
    AgtServer aServer(&loop, InetAddress(static_cast<uint16_t>(gConf.agtPort_)),
                      gConf.agtCmax_);
    aServer.start();

	// Add inspector entrance

	//ins.add()

	loop.loop();
}
