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
    DP_LERROR = 0x90,

    DP_DATAUP = 0x20,
    DP_DATAUPF = 0x21,

    DP_DATAACK = 0xa0,

} ;

struct DevInfoHeader {
    uint16_t len_;
    uint16_t type_;
    uint16_t devType_;
    uint8_t dID_[6];
    uint8_t con_[0];
};

struct DevBasic {
    uint16_t iCha_;
    uint16_t iRate_;
    uint16_t vRate_;
    uint8_t  modId_;
    uint8_t  status_;
};

struct DevErrHis {
    uint8_t  lastReason_;
    uint8_t  rev_;
    uint16_t duration_;
    uint8_t  time_[12];
};      // Used for DP_LERROR as well

struct DevErrHisF {
    uint8_t  lastReason_;
    uint8_t  rev_;
    uint16_t duration_;
    uint8_t  time_[12];
    uint16_t curr_[4];
};

struct DevData {
    uint8_t  ver_;
    uint8_t  status_;
    uint16_t rawData_[0];
};

struct DevDatawF {
    uint8_t  ver_;
    uint8_t  status_;
    uint8_t  lastReason_;
    uint8_t  rev_;
    uint16_t duration_;
    uint8_t  time_[12];
    uint16_t curr_[4];
    uint16_t rawData_[0];
};

#endif // DEVINFO_H
