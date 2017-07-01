#ifndef ZE_REDIS_WRAPPER_H
#define ZE_REDIS_WRAPPER_H

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

  void store(const muduo::StringPiece &cmd);

  int aSet(const muduo::StringPiece &cmd);

  void agentLogin(uint32_t);

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
