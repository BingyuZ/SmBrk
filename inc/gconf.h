/*
 * gconf.h
 *
 *  Created on: Dec 6, 2014
 *      Author: bingyu
 */

#ifndef GCONF_H_
#define GCONF_H_

#include <stdint.h>

#include <vector>
#include <string>

bool testEnvirn(void);

class GConf
{
public:
	int 		debug_;

	// Database servers
	std::string	rds1Addr_;	// Address of Redis server 1
	int16_t		rds1Port_;
	int16_t		rds1Thd_;   // Number of Redis server Threads. Ignored at present

	// Ports
    int16_t		agtPort_;   //
	int32_t		agtCmax_;   // Maximum concurrent connections for Devices
	int16_t     agtDHist_;

	int16_t		cmdPort_;
	int16_t		cmdCmax_;   // Maximum concurrent connections for Commands

	int16_t		hookPort_;
	int16_t		hookCmax_;  // Maximum concurrent connections for Hooks

	int16_t     monPort_;

public:
	bool SetPara(class ReadConf &r, std::string &reason);
	void PrintPara(void);
};

extern GConf gConf;

#endif /* GCONF_H_ */
