#include "servers.h"
#include "redisWrapper.h"
#include "commons.h"

#include "colors.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <memory.h>
#include <string>


#define WRITEREDISLOG

using namespace muduo;
using namespace muduo::net;
using namespace hiredis;

extern void PrintId(const uint8_t *p, char *tt);
static const char hexStr[] = "0123456789abcdef";
static const char *gRedisPass = "AUTH ZESmBrkTstSvr";

class SessID {
public:
    SessID(Session *pSess, const uint8_t *dId) : pSess_(pSess)
    {
        memcpy(Id_, dId, 6);
    }

    ~SessID() {}

    Session * pSess_;
    uint8_t Id_[6];
};


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
    LOG_DEBUG << "SetCmd " << ZEC_BLUE << redisReplyToString(reply) << ZEC_RESET;
}

void qConnectCallback(Hiredis* c, int status)
{
    if (status == REDIS_OK) {
        c->command(boost::bind(setCallback, _1, _2), gRedisPass);
    }
}

void redisQuery::connect(void)
{
    // AUTH Patch
    hRedis_.setConnectCallback(qConnectCallback);
    hRedis_.connect();
}

void redisStore::connect(void)
{
    // AUTH Patch
    hRedis_.setConnectCallback(qConnectCallback);
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

    sprintf(buf, "$%03d-%02d$N",
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
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    aSet(s);
}

void redisStore::agentLogin(uint32_t aid)
{
    // FIXME: Memory leakage???
    char buf[30], s[100];
    FormatZStr(buf, NULL, false, true);
    sprintf(s, "lpush agtLog:%08x 20%sZ$IN", aid, buf);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    aSet(s);
}

void redisStore::devLogin(const uint8_t *dId, uint32_t aid, uint8_t modId)
{
    char Sid[20], buf[30], s[100];

    PrintId(dId, Sid);
    FormatZStr(buf, NULL, false, false);

    sprintf(s, "lpush devLog:%s 20%s$IN$%08x$%02X",
                Sid, buf, aid, modId);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;

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
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::devProp(const uint8_t *dId, const struct DevBasic *pBasic, int len)
{
    char Sid[20], s[100];
    uint8_t *raw = (uint8_t *)pBasic;

    PrintId(dId, Sid);

    sprintf(s, "set devProp:%s %02X$", Sid, pBasic->ver_);
    char *s1 = s;
    while (*s1) ++s1;

    for (int i=2; i<len; i++){
        *s1++ = hexStr[raw[i] / 16];
        *s1++ = hexStr[raw[i] % 16];
    }
    *s1 = 0;

    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
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

    sprintf(s, "lpush devErr:%s %s$%02X$%d$%02X",
                Sid, t, err->lastReason_, dur, err->modId_);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::errHistF(const uint8_t *dId, const struct DevErrHisF *err)
{
    char Sid[20], t[20], s[200];
    uint16_t dur = err->duration_[0]*256 + err->duration_[1];

    if (dur == 0) return;   // if dur ==0, do NOT write to DB
    PrintId(dId, Sid);
    formatET(t, err->time_);

    sprintf(s, "lpush devErr:%s %s$%02X$%d$%02X$%d$%d$%d$%d",
                Sid, t, err->lastReason_, dur, err->modId_,
                err->curr_[0]*256+err->curr_[1], err->curr_[2]*256+err->curr_[3],
                err->curr_[4]*256+err->curr_[5], err->curr_[6]*256+err->curr_[7]);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
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

    sprintf(s, "lset devErr:%s 0 %s$%02X$%d$%02X$%d$%d$%d$%d",
                Sid, t, err->lastReason_, dur, err->modId_,
                err->curr_[0]*256+err->curr_[1], err->curr_[2]*256+err->curr_[3],
                err->curr_[4]*256+err->curr_[4], err->curr_[6]*256+err->curr_[7]);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

void redisStore::dataRpt(const uint8_t *dId, const DevData *pDev, int len)
{
    char Sid[20], buf[20], s[400], s2[50];
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
    sprintf(s2, "ltrim devData:%s 0 %d", Sid, gConf.agtDHist_-1);
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s2 << ZEC_RESET;
    #ifdef WRITEREDISLOG
    aSet(s2);
    #endif

    // Save date
    LOG_DEBUG << "RCmd: " << ZEC_BROWN << s << ZEC_RESET;
    #ifdef WRITEREDISLOG
    aSet(s);
    #endif
}

bool FillHist(DevErrHis *pHist, const char *ps, uint32_t len)
{
    int y,mo,d,h,m,s;
    if (len < 19 || ps[14] != '$' || ps[17] != '$' ) return false;
    sscanf(ps+2, "%2x%2x%2x%2x%2x%2x", &y, &mo, &d, &h, &m, &s);
    pHist->time_[0] = y;
    pHist->time_[1] = mo;
    pHist->time_[2] = d;
    pHist->time_[3] = h;
    pHist->time_[4] = m;
    pHist->time_[5] = s;
    //LOG_DEBUG << "Hist time:" << y<< mo << d << h << m << s;
    s = 0;
    sscanf(ps+15, "%X$%d$%X", &h, &m, &s);
    if (s == 0) return false;
    pHist->lastReason_ = h;
    pHist->duration_[0] = m/256;
    pHist->duration_[1] = m%256;
    pHist->modId_ = s;
    //LOG_DEBUG << h << m << s;
    return true;
}

void DevErrCB(hiredis::Hiredis* c, redisReply* reply, SessID * pSID)
{
    redisReply*item;

    if (reply->type != 2) {
        LOG_DEBUG << "Error while query lrange devErr";
    }
    else if (reply->element == 0) {
        LOG_DEBUG << "Cannot find devErr record";
        pSID->pSess_->sendDevHisN(pSID->Id_);
    }
    else {
        item = reply->element[0];

        char Sid[20];
        PrintId(pSID->Id_, Sid);
        LOG_DEBUG << Sid << " LR: "
                  << string(item->str, item->len);

        DevErrHis hist;
        bool ret = FillHist(&hist, item->str, item->len);
        if (ret) pSID->pSess_->sendDevHis(pSID->Id_, &hist);
        else pSID->pSess_->sendDevHisN(pSID->Id_);
    }

    delete pSID;
}

void redisQuery::checkLastErr(Session * pSess, const uint8_t * dId)
{
    char Sid[20];

    SessID *pSID = new SessID(pSess, dId);

    PrintId(dId, Sid);
    hRedis_.command(boost::bind(DevErrCB, _1, _2, pSID), "lrange devErr:%s 0 0", Sid);
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



