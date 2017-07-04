#ifndef ZE_REDIS_WRAPPER_H
#define ZE_REDIS_WRAPPER_H

#include "devInfo.h"
#include "Hiredis.h"


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
  void errHist(const uint8_t *, const struct DevErrHis *);
  void errHistF(const uint8_t *, const struct DevErrHisF *);
  void errMHist(const uint8_t *, const struct DevErrHisF *);

  void dataRpt(const uint8_t *, const uint8_t *,
               uint8_t ver, uint8_t status,  int);

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
    : hRedis(loop, serverAddr), loop_(loop) { }

  ~redisQuery() { }

  bool connected() const { return hRedis.connected(); }
  const char* errstr() const { return hRedis.errstr();  }

  void connect() { hRedis.connect(); }
  void disconnect() { hRedis.disconnect(); }  // FIXME: implement this with redisAsyncDisconnect

 private:
  Hiredis hRedis;
  muduo::net::EventLoop* loop_;
};

}

#endif //ZE_REDIS_WRAPPER_H
