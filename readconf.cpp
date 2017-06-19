/*
 * readconf.cpp
 *
 *  Created on: Dec 1, 2014
 *      Author: bingyu
 */

#include "commons.h"

#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>

#include "readconf.h"

using namespace std;

#define COMMENT_CHAR '#'

//	Read configuration file
//	Returns true when succeed
// 			false when error
//	Notes: Detailed information can be get by GetErrinfo
bool ReadConf::Read(void)
{
    KeyVal_.clear();

    ifstream infile(name_);

    if (!infile) return false;

    string line;
    while (getline(infile, line))
    	AnalyseLine(line);

    infile.close();
    return true;
}

static bool IsSpace(char c)
{
    if (' ' == c || '\t' == c) return true;
    return false;
}

static void Trim(string &str)
{
    if (str.empty())  return;

    unsigned int i, start_pos, end_pos;
    for (i = 0; i < str.size(); ++i)
    	if (!IsSpace(str[i])) break;

    if (i == str.size())
    { 	// string of spaces
        str = "";
        return;
    }
    start_pos = i;

    for (i = str.size() - 1; i >= 0; --i)
        if (!IsSpace(str[i])) break;
    end_pos = i;

    str = str.substr(start_pos, end_pos - start_pos + 1);
}

void ReadConf::AnalyseLine(const string & line)
{
    if (line.empty()) return;

    int end_pos = line.size(), pos;

    if ((pos = line.find(COMMENT_CHAR)) != -1) {
        if (0 == pos)  return;   // The first character is a comment char
        else end_pos = pos;
    }

    string new_line = line.substr(0, end_pos);  // delete comments

    if ((pos = new_line.find('=')) == -1) return;	// No '='	string s;

    string key, value;

    key = new_line.substr(0, pos);
    value = new_line.substr(pos + 1, end_pos - pos);

    Trim(key);
    if (key.empty()) return;

    std::transform(key.begin(), key.end(), key.begin(), ::toupper);	// transform key to upper case

    Trim(value);
    KeyVal_[key] = value;
}

// Returns true if key is found, result is stored in value
// otherwise returns false;
bool ReadConf::GetValue(string &key, string &value)
{
	map<string, string>::const_iterator it = KeyVal_.find(key);
	if (it == KeyVal_.end()) return false;

	value = KeyVal_[key];
	return true;
}

// Returns true if key is found, result is stored in value
// otherwise returns false;
bool ReadConf::GetValue(const char *s, string &value)
{
	string key(s);
	map<string, string>::const_iterator it = KeyVal_.find(key);
	if (it == KeyVal_.end()) return false;

	value = KeyVal_[key];
	return true;
}

// Test purpose
// Print active contents in config file
void ReadConf::PrintConfig(void)
{
	cout << "Config file as follows:" << endl;
    map<string, string>::const_iterator mite = KeyVal_.begin();
    for (; mite != KeyVal_.end(); ++mite) {
        cout << mite->first << ":" << mite->second << endl;
    }
}
