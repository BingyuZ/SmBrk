#include "redisWrapper.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <string>
#include "commons.h"

#define WRITEREDISLOG

using namespace muduo;
using namespace muduo::net;
using namespace hiredis;

extern void PrintId(const uint8_t *p, char *tt);
static const char hexStr[] = "0123456789abcdef";

const int gMaxRecord = 119;

void formatET(char *t, const uint8_t *eT)
{
    *t++ = '2';
    *t++ = '0';
    for (int i=0; i<6; i++) {
        *t++ = hexStr[*eT / 16];
        *t++ = hexStr[*eT % 16];
        eT++;
    }
    *t = 0;
}

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
    char buf[30], buf1[100], s[200];
    FormatZStr(buf, NULL, false, true);
    formatGPRS(buf1, pGprs);
    sprintf(s, "lpush agtLog:%08x 20%sZ$IN%s", aid, buf, buf1);
    LOG_DEBUG << "Redis command: " << s;
    aSet(s);
}

void redisStore::agentLogin(uint32_t aid)
{
    // FIXME: Memory leakage???
    char buf[30], s[100];
    FormatZStr(buf, NULL, false, true);
    sprintf(s, "lpush agtLog:%08x 20%sZ$IN", aid, buf);
    LOG_DEBUG << "Redis command: " << s;
    aSet(s);
}

void redisStore::devLogin(const uint8_t *dId, uint32_t aid, uint8_t modId)
{
    char Sid[20], buf[30], s[100];

    PrintId(dId, Sid);
    FormatZStr(buf, NULL, false, false);

    sprintf(s, "lpush devLog:%s 20%s$IN$%08x$%d",
                Sid, buf, aid, modId);
    LOG_DEBUG << "Redis command: " << s;

    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::devLogout(const uint8_t *dId)
{
    char Sid[20], buf[30], s[100];

    PrintId(dId, Sid);
    FormatZStr(buf, NULL, false, false);

    sprintf(s, "lpush devLog:%s 20%s$ON", Sid, buf);
    LOG_DEBUG << "Redis command: " << s;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}


void redisStore::errHist(const uint8_t *dId, const struct DevErrHis *err)
{
    char Sid[20], t[20], s[100];
    uint16_t dur = err->duration_[0]*256 + err->duration_[1];

    PrintId(dId, Sid);
    formatET(t, err->time_);

    sprintf(s, "lpush devErr:%s %s$%02X$%.2f",
                Sid, t, err->lastReason_, dur/100.0);
    LOG_DEBUG << "Redis command: " << s;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::errHistF(const uint8_t *dId, const struct DevErrHisF *err)
{
    char Sid[20], t[20], s[200];
    uint16_t dur = err->duration_[0]*256 + err->duration_[1];

    PrintId(dId, Sid);
    formatET(t, err->time_);

    sprintf(s, "lpush devErr:%s %s$%02X$%.2f$d%d$%d$%d$%d",
                Sid, t, err->lastReason_, dur/100.0,
                err->curr_[0]*256+err->curr_[1], err->curr_[2]*256+err->curr_[3],
                err->curr_[4]*256+err->curr_[5], err->curr_[6]*256+err->curr_[7]);
    LOG_DEBUG << "Redis command: " << s;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::errRHist(const uint8_t *dId, const struct DevErrHisF *err)
{
    char Sid[20], t[20], s[200];
    uint16_t dur = err->duration_[0]*256 + err->duration_[1];

    PrintId(dId, Sid);
    formatET(t, err->time_);

    sprintf(s, "lset devErr:%s 0 %s$%02X$%.2f$d%d$%d$%d$%d",
                Sid, t, err->lastReason_, dur/100.0,
                err->curr_[0]*256+err->curr_[1], err->curr_[2]*256+err->curr_[3],
                err->curr_[4]*256+err->curr_[4], err->curr_[6]*256+err->curr_[7]);
    LOG_DEBUG << "Redis command: " << s;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::dataRpt(const uint8_t *dId, const DevData *pDev, int len)
{
    char Sid[20], buf[20], s[200], s2[50];
    int i;

    PrintId(dId, Sid);
    FormatZStr(buf, NULL, false, false);

    sprintf(s, "lpush devData:%s %s$%02X$%02X", Sid, buf, pDev->status_, pDev->ver_);
    char *s1 = s;
    while (*s1) ++s1;

    for (i=0; i<len; i++){
        *s1++ = hexStr[pDev->rawData_[i] / 16];
        *s1++ = hexStr[pDev->rawData_[i] % 16];
    }
    *s1 = 0;

    // Add restrict
    sprintf(s2, "ltrim devData:%s 0 %d", Sid, gMaxRecord-1);
    LOG_DEBUG << "Redis command: " << s2;
    #ifdef WRITEREDISLOG
    aSet(s2);
    #endif

    // Save date
    LOG_DEBUG << "Redis command: " << s;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void DevErrCB(hiredis::Hiredis* c, redisReply* reply, Session * pSess)
{
    redisReply*item;
    if (reply->type != 2) {
        LOG_DEBUG << "Error while query lrange devErr";
    }
    else if (reply->element == 0) {
        LOG_DEBUG << "Cannot find devErr record";
            // TODO: No such record
    }
    else {
        item = reply->element[0];
        LOG_INFO << string(item->str, item->len);
        // TODO: reply
    }
}

void redisQuery::checkLastErr(Session * pSess, const uint8_t * dId)
{
    char Sid[20];

    PrintId(dId, Sid);
    hRedis_.command(boost::bind(DevErrCB, _1, _2, pSess), "lrange devErr:%s 0 0", Sid);
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



