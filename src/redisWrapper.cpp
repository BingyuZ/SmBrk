#include "redisWrapper.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <string>

using namespace muduo;
using namespace muduo::net;
using namespace hiredis;

string toString(long long value)
{
  char buf[32];
  snprintf(buf, sizeof buf, "%lld", value);
  return buf;
}

string redisReplyToString(const redisReply* reply)
{
  static const char* const types[] = { "",
      "REDIS_REPLY_STRING", "REDIS_REPLY_ARRAY",
      "REDIS_REPLY_INTEGER", "REDIS_REPLY_NIL",
      "REDIS_REPLY_STATUS", "REDIS_REPLY_ERROR" };
  string str;
  if (!reply) return str;

  str += types[reply->type] + string("(") + toString(reply->type) + ") ";

  str += "{ ";
  if (reply->type == REDIS_REPLY_STRING ||
      reply->type == REDIS_REPLY_STATUS ||
      reply->type == REDIS_REPLY_ERROR)
  {
    str += '"' + string(reply->str, reply->len) + '"';
  }
  else if (reply->type == REDIS_REPLY_INTEGER)
  {
    str += toString(reply->integer);
  }
  else if (reply->type == REDIS_REPLY_ARRAY)
  {
    str += toString(reply->elements) + " ";
    for (size_t i = 0; i < reply->elements; i++)
    {
      str += " " + redisReplyToString(reply->element[i]);
    }
  }
  str += " }";

  return str;
}

void setCallback(Hiredis* c, redisReply* reply)
{
    LOG_DEBUG << "SetCmd " << redisReplyToString(reply);
}

int redisStore::setPair(const std::string &key, const std::string &value)
{
    LOG_DEBUG << "Set " << key << " " << value;
    return hRedis.command(boost::bind(setCallback, _1, _2), "set %s \"%s\"",
                            key.c_str(), value.c_str());
}

// mainly for HMSET
int redisStore::genSet(const std::string &cmd)
{
    LOG_DEBUG << "GeneralCommand " << cmd;
    return hRedis.command(boost::bind(setCallback, _1, _2), cmd);
}

int redisStore::zAdd(const std::string &key, int weight, const std::string &value)
{
    LOG_DEBUG << "Zadd " << key << " " << weight << " " << value;
    return hRedis.command(boost::bind(setCallback, _1, _2), "zadd %s %d \"%s\"",
                            key.c_str(), weight, value.c_str());
}




