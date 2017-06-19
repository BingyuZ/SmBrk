/*
 * readconf.h
 *
 *  Created on: Dec 2, 2014
 *      Author: bingyu
 */

#ifndef READCONF_H_
#define READCONF_H_

#include <map>
#include <string>

using std::map;
using std::string;

class ReadConf
{
private:
	std::map<string, string> KeyVal_;		// Should not explose to outside
	const char *name_;

public:
	ReadConf(void) { }
	void SetName(const char *fileName) { name_ = fileName; }
	bool Read(void);
	bool GetValue(string &key, string &value);
	bool GetValue(const char *s, string &value);
	const char *GetFileName(void) { return name_; }
	void PrintConfig(void);
	~ReadConf() { };

private:
	void AnalyseLine(const string & line);

};


#endif /* READCONF_H_ */
