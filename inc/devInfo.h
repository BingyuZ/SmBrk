/*
 * devinfo.h
 *
 *  Created on: July 1, 2017
 *      Author: Bingyu
 */
#ifndef DEVINFO_H
#define DEVINFO_H

#include <stdint.h>
enum DevPkType {
    DP_UNKNOWN = 0,

    DP_BASIC = 0x10,
    DP_LOST = 0x11,

    DP_ERRHIS = 0x14,
    DP_ERRHISF = 0x15,

    DP_ACK = 0x80,
    DP_LASTHIS = 0x81,
    DP_LASTNAK = 0x82,

    DP_LERROR = 0x90,

    DP_DATAUP = 0x20,
    DP_DATAUPF = 0x21,
    DP_DATAUPG = 0x22,

    DP_DATAACK = 0xa0,

} ;

struct DevInfoHeader {
    uint8_t len_;
    uint8_t type_;
    //uint16_t devType_;
    uint8_t dID_[6];
    uint8_t con_[0];
};

struct DevBasic {
	uint8_t  ver_;
    uint8_t  modId_;
    uint16_t iCha_;
    uint16_t iRate_;
    uint16_t vRate_;
};

struct DevErrHis {
    uint8_t  lastReason_;
    uint8_t  modId_;
    uint8_t  duration_[2];
    uint8_t  time_[6];
    uint16_t rev2_;
};      // Used for DP_LERROR as well

struct DevErrHisF {
    uint8_t  lastReason_;
    uint8_t  modId_;
    uint8_t  duration_[2];
    uint8_t  time_[6];
    uint8_t  curr_[8];
    uint16_t rev2_;
};

struct DevData {
    uint8_t  ver_;
    uint8_t  status_;
    uint8_t  rawData_[0];
};

struct DevDatawF {
    uint8_t  lastReason_;
    uint8_t  modId_;
    uint8_t  duration_[2];
    uint8_t  time_[12];
    uint8_t  curr_[8];
    uint16_t rev2_;
    uint8_t  ver_;
    uint8_t  status_;
    uint8_t  rawData_[0];
};

struct GPRSInfo {
    uint8_t  IMEI_[8];
    uint8_t  opid_[4];
    uint8_t  ccid_[10];
    uint8_t  lac_[2];
    uint8_t  ci_[2];
    int8_t   asu_;
    uint8_t  ber_;
};



struct HookHeader {
    uint8_t len_[2];
    uint8_t dId_[6];
};


enum cmdType {
    CMD_UNKNOWN = 0,

    CMD_GERREGS = 0x10,
    CMD_SETREGS = 0x20,
    CMD_MASKREGS = 0x30,

    CMD_ACK = 0x90,
    CMD_DEVLOST = 0xa0,
    CMD_REJECT = 0xa1,
};

struct CmdReqs {
    uint8_t len_;
    uint8_t type_;
    uint8_t dID_[6];
    uint8_t numRegs_;
    uint8_t regType_;
    uint8_t startReg_[2];
    uint8_t value_[0];
};

struct CmdReply {
    // General Header
    uint8_t len_;
    uint8_t type_;
    uint8_t dID_[6];
    // Request
    uint8_t numRegs_;
    uint8_t regType_;
    uint8_t startReg_[2];
    // Result
    uint8_t result_[4];
    uint8_t data_[0];
};

#endif // DEVINFO_H
