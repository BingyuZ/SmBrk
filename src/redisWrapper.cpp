#include "redisWrapper.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <string>
#include "commons.h"

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

void redisStore::connect(void)
{
    loop_->runInLoop(boost::bind(&Hiredis::connect, &hRedis_));
}

void redisStore::store(const muduo::string &cmd)
{
    hRedis_.command(boost::bind(setCallback, _1, _2), cmd.data());
}

// Firewall :)
int redisStore::aSet(const muduo::string &cmd)
{
  if (!connected()) {
    LOG_WARN << "Not connected";
    return REDIS_ERR;
  }

  loop_->runInLoop(boost::bind(&redisStore::store, this, cmd));
  return 0;
}

void redisStore::agentLogin(uint32_t aid)
{
    // FIXME: Memory leakage???
    char buf[30], s[100];
    FormatZStr(buf, NULL, false, true);
    sprintf(s, "lpush agtLogin:%08x IN20%sZ", aid, buf);
    LOG_DEBUG << "Redis command: " << s;
    aSet(s);
}

static const char hexStr[] = "0123456789abcdef";

static void formatGPRS(char *buf, const GPRSInfo* pGprs)
{
    int i;

    *buf++='$';
    *buf++='I';
    for (i=0; i<7; i++) {
        *buf++ = hexStr[pGprs->IMEI_[i]/16];
        *buf++ = hexStr[pGprs->IMEI_[i]%16];
    }
    *buf++ = hexStr[pGprs->IMEI_[7]/16];

    sprintf(buf, "$O%03d-%02d$N",
            pGprs->opid_[0]*256+pGprs->opid_[1], pGprs->opid_[3]);
    while (*buf) ++buf;

    for (i=0; i<10; i++) {
        *buf++ = hexStr[pGprs->ccid_[i]/16];
        *buf++ = hexStr[pGprs->ccid_[i]%16];
    }

    sprintf(buf, "$L%02x%02x$C%02x%02x$A%d$B%d",
            pGprs->lac_[0], pGprs->lac_[1], pGprs->ci_[0], pGprs->ci_[1],
            pGprs->asu_, pGprs->ber_);
}

void redisStore::agentLogin(uint32_t aid, const GPRSInfo* pGprs)
{
    // FIXME: Memory leakage???
    char buf[30], buf1[100], s[200];
    FormatZStr(buf, NULL, false, true);
    formatGPRS(buf1, pGprs);
    sprintf(s, "lpush agtLogin:%08x IN20%sZ%s", aid, buf, buf1);
    LOG_DEBUG << "Redis command: " << s;
    aSet(s);
}


#if 0
void redisStore::setPair(const muduo::StringPiece &key, const muduo::StringPiece &value)
{
    hRedis_.command(setCallback, "set %s \"%s\"",
                    key.data(), value.data());

    loop_->runInLoop(boost::bind(&Hiredis::connect, &hRedis_));
// TODO: Above test!!!

    loop_->runInLoop(boost::bind(&Hiredis::command, &hRedis_,
                                 boost::bind(setCallback, _1, _2),
                                 "set %s \"%s\"", key.data(), value.data()));

//    LOG_DEBUG << "Set " << key << " " << value;
//    return hRedis.command(boost::bind(setCallback, _1, _2), "set %s \"%s\"",
//                            key.c_str(), value.c_str());
}

// mainly for HMSET
void redisStore::genSet(const muduo::StringPiece &cmd)
{
    loop_->runInLoop(boost::bind(&Hiredis::command, &hRedis_,
                                 boost::bind(setCallback, _1, _2),
                                 cmd.data()));

//    LOG_DEBUG << "GeneralCommand " << cmd;
//    return hRedis.command(boost::bind(setCallback, _1, _2), cmd);
}

void redisStore::zAdd(const muduo::StringPiece &key, int weight, const muduo::StringPiece &value)
{
    loop_->runInLoop(boost::bind(&Hiredis::command, &hRedis_,
                                 boost::bind(setCallback, _1, _2),
                                 "zadd %s %d \"%s\"", key.data(), weight, value.data()));

//    LOG_DEBUG << "Zadd " << key << " " << weight << " " << value;
//    return hRedis.command(boost::bind(setCallback, _1, _2), "zadd %s %d \"%s\"",
//                            key.c_str(), weight, value.c_str());
}
#endif



