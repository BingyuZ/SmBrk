#ifndef ZE_REDIS_WRAPPER_H
#define ZE_REDIS_WRAPPER_H

#include "Hiredis.h"

namespace hiredis
{

class redisStore : public boost::enable_shared_from_this<Hiredis>,
                boost::noncopyable
{
 public:
  redisStore(muduo::net::EventLoop* loop, const muduo::net::InetAddress& serverAddr)
    : hRedis(loop, serverAddr), loop_(loop) { }

  ~redisStore() { }

  bool connected() const { return hRedis.connected(); }
  const char* errstr() const { return hRedis.errstr();  }

  void connect() { hRedis.connect(); }
  void disconnect() { hRedis.disconnect(); }  // FIXME: implement this with redisAsyncDisconnect

  int setPair(const std::string &key, const std::string &value);
  int genSet(const std::string &cmd);
  int zAdd(const std::string &key, int weight, const std::string &value);

 private:
  Hiredis hRedis;
  muduo::net::EventLoop* loop_;
};


class redisQuery : public boost::enable_shared_from_this<Hiredis>,
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
