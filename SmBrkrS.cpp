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

#include <string.h>
#include <unistd.h>

#include "iostream"


using namespace muduo;
using namespace muduo::net;

using namespace std;
//using namespace zeinmod;

#include "gconf.h"
#include "commons.h"


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
    cout << "DONE!" << endl;
}
