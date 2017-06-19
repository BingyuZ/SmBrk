#ifndef CONNMAN_H
#define CONNMAN_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <map>

enum {
    SES_UNUSED  = 0,
    SES_CLEAR,
    SES_VALID,
    SES_OUTDATED
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
    InitSessionPool(void) {
        sPool_.reserve(nsPool_);
		for (int i=0; i<nsPool_; i++) {
            sPool_[i].status = SES_UNUSED;
			sFree_.push(i);
		}
    }

public:
    ConnMan(int32_t maxSession) : nsPool_(maxSession) {
        InitSessionPool();
    }

    virtual ~ConnMan() {}

    int32_t GetSessionID

    int32_t NewSession(uint32_t aID) {

        std::set<uint32_t>::iterator it = agtSet_.find(aID);
        if (it != agtSet_.end()) return -1;     // Agent existed

        if (sFree_.empty()) return -1;

    }

    void EndSession(int32_t sID) {
        sID -= kOffset;
        assert(sID >= 0 && sID < nsPool_);

        // TODO: agentID
        sFree_.push(sID);
        sPool_[sID].status_ = SES_UNUSED;
    }

    Session *GetSession(int32_t sID) {
        sID -= kOffset;
        if (sID < 0 || sID >= nsPool_) return NULL;
        if (sPool_[sID].status_ == SES_UNUSED)  return NULL;

        return &(sPool_[sID]);
    }


private:
    const static int32_t kOffset = 0x2000;

    int32_t nsPool_;

    std::vector<Session> sPool_;    // Session Pool
	std::queue<int32_t> sFree_;     // Unused session

	std::map<uint32_t, int32_t>  agtMap_;
};

extern ConnMan theConnMan;

#endif // CONNMAN_H
