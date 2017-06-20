#ifndef CONNMAN_H
#define CONNMAN_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <stdint.h>

#include <map>
#include <vector>
#include <queue>
#include <stack>

enum {
    SES_UNUSED  = 0,
    SES_CLEAR,
    SES_VALID,
    SES_OUTDATED,
};

struct Session
{
    uint32_t    agentID_;
    uint32_t    password_[4];   // 128 bit
    TcpConnectionPtr tcpCon_;
    int         status_;
};



class ConnMan
{
private:
    void InitSessionPool(void) {
        sPool_.reserve(nsPool_+2);
        //svFree_.reserve(nsPool_+2);
		for (int i=0; i<nsPool_; i++) {
            sPool_[i].status_ = SES_UNUSED;
            sPool_[i].tcpCon_ = NULL;
            sPool_[i].agentID_ = 0;
			sFree_.push(i);
		}
    }

public:
    ConnMan(int32_t maxSession) : nsPool_(maxSession) {
        InitSessionPool();
    }

    virtual ~ConnMan() {}

    // Get a session ID from an agentID
    // Returns -1 if not found
    int32_t GetSessionID(uint32_t aID) {
        std::map<uint32_t, int32_t>::const_iterator it = agtMap_.find(aID);
        if (it == agtMap_.end()) return -1;     // Agent not found

        int32_t sID = it->second;
        assert(sID >= 0 && sID < nsPool_);
        assert(sPool_[sID].status_ == SES_UNUSED);
        return sID;
    }

    // New session from an agent ID.
    // Returns a free sessionID on success
    // Returns -1 on error
    int32_t NewSession(uint32_t aID) {
        std::map<uint32_t, int32_t>::const_iterator it = agtMap_.find(aID);
        if (it != agtMap_.end()) return -1;     // Agent existed

        MutexLockGuard lock(mutex_);

        if (sFree_.empty()) return -1;          // No SessionID available
        int32_t sID = sFree_.top();
        assert(sID >= 0 && sID < nsPool_);
        sFree_.pop();
        assert(sPool_[sID].status_ == SES_UNUSED);

        agtMap_.insert(std::map<uint32_t, int32_t>::value_type(aID, sID));
        sPool_[sID].agentID_ = aID;
        sPool_[sID].status_ = SES_CLEAR;
        return sID;
    }

    // Ends a session and returns the session ID
    void EndSession(int32_t sID) {
        //sID -= kOffset;
        assert(sID >= 0 && sID < nsPool_);

        MutexLockGuard lock(mutex_);

        sFree_.push(sID);

        sPool_[sID].password_[0] = 0;
        sPool_[sID].agentID_ = 0;
        //sPool_[sID].tcpCon_ = NULL;
        sPool_[sID].status_ = SES_UNUSED;

        agtMap_.erase(sID);
    }

    // Get a session object by session ID
    // sessionID is between kOffset & kOffset + maxSession
    // Returns NULL is not found
    Session *GetSession(int32_t sID) {
        //sID -= kOffset;
        if (sID < 0 || sID >= nsPool_) return NULL;
        if (sPool_[sID].status_ == SES_UNUSED)  return NULL;

        return &(sPool_[sID]);
    }


private:
    //const static int32_t kOffset = 0x2000;

    const int32_t nsPool_;

    std::vector<Session> sPool_;            // Session Pool

    //std::vector<int32_t> svFree_;
    std::stack<int32_t, std::vector<int32_t> > sFree_;    // Unused sessionID

	std::map<uint32_t, int32_t>  agtMap_;   // Mapping of agent ID and session ID
                                            // agentID sessionID
    MutexLock 	mutex_;
};

extern ConnMan theConnMan;

#endif // CONNMAN_H
