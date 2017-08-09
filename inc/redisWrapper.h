#ifndef ZE_REDIS_WRAPPER_H
#define ZE_REDIS_WRAPPER_H

//#include "devInfo.h"
#include "Hiredis.h"

class Session;
struct GPRSInfo;
struct DevErrHis;
struct DevErrHisF;
struct DevData;
struct DevBasic;

namespace hiredis
{

class redisStore : public boost::enable_shared_from_this<redisStore>,
                boost::noncopyable
{
 public:
  redisStore(muduo::net::EventLoop* loop, const muduo::net::InetAddress& serverAddr)
    : hRedis_(loop, serverAddr), loop_(loop) { }
  ~redisStore() { }


  const char* errstr() const { return hRedis_.errstr();  }

  void connect(void);

  void store(const muduo::string &cmd);

  int aSet(const muduo::string &cmd);

  void agentLogin(uint32_t);
  void agentLogin(uint32_t, const GPRSInfo*);
  void devLogin(const uint8_t *, uint32_t, uint8_t);
  void devLogout(const uint8_t *);
  void devProp(const uint8_t *, const struct DevBasic *, int);
  void errHist(const uint8_t *, const struct DevErrHis *);
  void errHistF(const uint8_t *, const struct DevErrHisF *);
  void errRHist(const uint8_t *, const struct DevErrHisF *);

  void dataRpt(const uint8_t *, const DevData *pDev, int len);

  //void setPair(const muduo::StringPiece &key, const muduo::StringPiece &value);
  //void genSet(const muduo::StringPiece &cmd);
  //void zAdd(const muduo::StringPiece &key, int weight, const muduo::StringPiece &value);

 private:
  bool connected() const { return hRedis_.connected(); }
  void disconnect() { hRedis_.disconnect(); }

  Hiredis hRedis_;
  muduo::net::EventLoop* loop_;
};


class redisQuery : public boost::enable_shared_from_this<redisQuery>,
                boost::noncopyable
{
 public:
  redisQuery(muduo::net::EventLoop* loop, const muduo::net::InetAddress& serverAddr)
    : hRedis_(loop, serverAddr), loop_(loop) { }

  ~redisQuery() { }

  bool connected() const { return hRedis_.connected(); }
  const char* errstr() const { return hRedis_.errstr();  }

  void connect() { hRedis_.connect(); }
  void disconnect() { hRedis_.disconnect(); }  // FIXME: implement this with redisAsyncDisconnect

  void checkLastErr(Session * pSess, const uint8_t *);

 private:
  Hiredis hRedis_;
  muduo::net::EventLoop* loop_;
};

}

#endif //ZE_REDIS_WRAPPER_H
