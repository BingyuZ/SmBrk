//============================================================================
// Name        : SmartBreakerMain.cpp
// Author      : Bingyu
// Version     :
// Copyright   : ZEin 2017
// Description :
//============================================================================

#define _VERSION_	"V0.10"

#include "commons.h"

#include <muduo/base/Logging.h>
#include <muduo/base/CurrentThread.h>

#include "iostream"

#include "readconf.h"

using namespace std;

void usage(int ret);
void SBSMain(void);

const char *gAppName;
static const char *defaultConf = "/etc/zein/smbrksys.conf";

int main(int argc, char *argv[]) {

	int ch;
	bool testonly = false;

	ReadConf rConf;
	string reason;

	cout << "ZEIN SmartBreaker Server " << _VERSION_ << "b " << __DATE__
		 << " " << __TIME__ << endl << endl;

	gAppName = argv[0];

	// Uncomment this if you want to write log to local file instead of console
//	SetLogToFile(gAppName);

	LOG_INFO << "pid = " << getpid() << ", tid = " << "" << muduo::CurrentThread::tid();

	rConf.SetName(defaultConf);
	while ((ch = getopt(argc, argv, "c:t")) != -1)
	{
		switch(ch)
		{
			case 'c':
				rConf.SetName(optarg);
				break;
			case 't':
				testonly = true;
				break;
			default:
				usage(1);
		}	// end of switch(ch)
	}	// end of while getopt
	argc -= optind;
	argv += optind;
	//	Make sure options are clean and used properly

	//	Read configuration file
	if (rConf.Read() == false)
	{
		string s = "Config file ";
		s += rConf.GetFileName();
		s += " not found";
		errQuit(2, s.c_str());
	}

	if (gConf.SetPara(rConf, reason) == false) errQuit(2, reason.c_str());

	muduo::Logger::setLogLevel(
			static_cast<enum muduo::Logger::LogLevel>(gConf.debug_));

	LOG_TRACE << "Configure File OK";

	if (testonly)
	{
		gConf.PrintPara();
		return 0;
	}

	if (testEnvirn())
	{
		// TODO: Uncomment this if you want to make this program as a service
		// daemon(1,1);
		SBSMain();
		return 0;
	}
	else
	{
		cout << "Server name cannot be resolved\n";
	}
	return -1;
}


void usage(int ret)
{
	cerr << "Usage: " << gAppName << endl;
	cerr << "\t[-c ConfigFile]    Specify config file other than "
		 <<	defaultConf << endl;
	cerr << "\t[-t]               Verify config file and quit" << endl;
	cerr << endl;
	if (ret)
		exit(1);
}
