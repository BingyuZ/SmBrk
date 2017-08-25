/*
 * Reqs.h
 *
 *  Created on: Aug 1, 2017
 *      Author: Bingyu
 */

#ifndef ZE_REQS_H
#define ZE_REQS_H

#include "devInfo.h"
#include <string.h>

#include <map>

#include <muduo/base/Mutex.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

using std::map;

class ReqItem {
public:
    ReqItem(const CmdReqs *pReq)
    {
        memcpy(&req_, pReq, sizeof(CmdReqs));
    }
    ~ReqItem() {}

private:
    CmdReqs req_;

};

typedef boost::shared_ptr<ReqItem> ReqItemPtr;
typedef std::map<uint64_t, ReqItemPtr> ItemMap;

class ReqMan {
public:
    ReqMan(void) {}

    ~ReqMan() {}

    uint64_t AddReq(const CmdReqs*);

private:
    muduo::MutexLock 	mutex_;
    ItemMap map_;

};


#endif
