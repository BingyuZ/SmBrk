/*
 * gconf.cpp
 *
 *  Created on: Dec 13th, 2016
 *      Author: bingyu
 */

#include "gconf.h"
#include "readconf.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <hiredis/hiredis.h>

#include <string>
#include <iostream>


using namespace std;

GConf gConf;


bool testEnvirn(void)
{
    //return true;
    redisContext *c;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(gConf.rds1Addr_.c_str(), gConf.rds1Port_, timeout);

    if (c == NULL || c->err) {
        if (c) {
            cout << "Connection error: " << c->errstr << endl;
            redisFree(c);
        } else {
            cout << "Connection error: can't allocate redis context" << endl;
        }
        return false;
    }
    cout << "Redis server OK\n";
    redisFree(c);
	return true;
}


void GConf::PrintPara(void)
{
	cout << "Configure file contents:" << endl;
	cout << "Debug level: " << debug_ << endl;

	cout << "Redis server address: " << rds1Addr_ << " port: " << rds1Port_ << endl;
	//cout << "Maximum Redis connections: " << rds1Thrd_ << endl;

	cout << "Port for devices: " << agtPort_ << endl;
	cout << "Maximum concurrent connections for devices: " << agtCmax_ << endl;
	cout << "Maximum data history of a device: " << agtDHist_ << endl;

	cout << "Port for command agents: " << cmdPort_ << endl;
	cout << "Maximum concurrent connections for command agents: " << cmdCmax_ << endl;

	cout << "Port for hooks: " << hookPort_ << endl;
	cout << "Maximum concurrent connections for hooks: " << hookCmax_ << endl;

	cout << "Monitoring port: " << monPort_ << endl;

	cout << "Idle interval: " << idleSeconds_ << endl;
}

bool GConf::SetPara(ReadConf &r, string &reason)
{
	string v;

	if (r.GetValue("RDS1ADDR", v)) rds1Addr_ = v;
	else { reason = "RDS1ADDR wrong."; return false; }
	if (r.GetValue("RDS1PORT", v)) rds1Port_ = atoi(v.c_str()); else rds1Port_ = 6379;
	if (rds1Port_ <= 0 || rds1Port_> 32000)	{ reason = "RDS1PORT wrong.";	return false;	}
	if (r.GetValue("RDS1THRD", v)) rds1Thd_ = atoi(v.c_str()); else rds1Thd_ = 10;
	if (rds1Thd_ <= 0 || rds1Thd_> 50)	{ reason = "RDS1THRD wrong.";	return false;	}

	if (r.GetValue("AGTPORT", v)) agtPort_ = atoi(v.c_str()); else agtPort_ = 6567;
	if (agtPort_ <= 0 || agtPort_> 32000)	{ reason = "AGTPORT wrong.";	return false;	}
	if (r.GetValue("AGTMAX", v)) agtCmax_ = atoi(v.c_str()); else agtCmax_ = 5000;
	if (agtCmax_ <= 0 || agtCmax_> 20000)	{ reason = "AGTMAX wrong.";	return false;	}
	if (r.GetValue("AGTDHIST", v)) agtDHist_ = atoi(v.c_str()); else agtDHist_ = 120;
	if (agtDHist_ <= 5 || agtDHist_> 1000)	{ reason = "AGTDHIST wrong.";	return false;	}


	if (r.GetValue("CMDPORT", v)) cmdPort_ = atoi(v.c_str()); else cmdPort_ = 6668;
	if (cmdPort_ <= 0 || cmdPort_> 32000)	{ reason = "CMDPORT wrong.";	return false;	}
	if (r.GetValue("CMDMAX", v)) cmdCmax_ = atoi(v.c_str()); else cmdCmax_ = 50;
	if (cmdCmax_ <= 0 || cmdCmax_> 100)	{ reason = "CMDMAX wrong.";	return false;	}

	if (r.GetValue("HOOKPORT", v)) hookPort_ = atoi(v.c_str()); else hookPort_ = 6569;
	if (hookPort_ <= 0 || hookPort_> 32000)	{ reason = "HOOKPORT wrong.";	return false;	}
	if (r.GetValue("HOOKMAX", v)) hookCmax_ = atoi(v.c_str()); else hookCmax_ = 2;
	if (hookCmax_ <= 0 || hookCmax_> 6)	{ reason = "HOOKMAX wrong.";	return false;	}

	if (r.GetValue("MONPORT", v)) monPort_ = atoi(v.c_str()); else hookPort_ = 6688;
	if (hookPort_ <= 0 || hookPort_> 32000)	{ reason = "MONPORT wrong.";	return false;	}

	if (r.GetValue("IDLESEC", v)) idleSeconds_ = atoi(v.c_str()); else idleSeconds_ = 90;
	if (idleSeconds_ <= 10 || idleSeconds_> 600)	{ reason = "IDLESEC wrong.";	return false;	}


	if (r.GetValue("DEBUG", v)) debug_ = atoi(v.c_str());	else debug_ = 0;

	return true;
}

