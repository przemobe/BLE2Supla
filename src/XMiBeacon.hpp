/*
 * SPDX-FileCopyrightText: 2026 Przemyslaw Bereski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <stdint.h>
#include <cstdio>


#define XMIBEACON_TAG               "XMiBeacon"
#define XMIBEACON_NONCE_LEN         (12)
#define XMIBEACON_RANDNUM_LEN       (3)
#define XMIBEACON_MIC_LEN           (4)
#define XMIBEACON_AUTHDATA_LEN      (1)
#define XMIBEACON_BINDKEY_BIT_LEN   (128)
#define XMIBEACON_MAC_LEN           (6)

const static uint16_t XMIBEACON_UUID = 0xfe95;
const static uint8_t XMIBEACON_AUTHDATA = 0x11;

#define XMIBEACON_FRAMECTRL_BYTE0   (0)
#define XMIBEACON_FRAMECTRL_BYTE1   (1)
#define XMIBEACON_PRODID_BYTE0      (2)
#define XMIBEACON_PRODID_BYTE1      (3)
#define XMIBEACON_FRAMECTR_BYTE0    (4)

#define XMIBEACON_FRAMECTRL_ISENCR_MSK  (0x0008)
#define XMIBEACON_FRAMECTRL_MACINC_MSK  (0x0010)
#define XMIBEACON_FRAMECTRL_CAPINC_MSK  (0x0020)
#define XMIBEACON_FRAMECTRL_OBJINC_MSK  (0x0040)
#define XMIBEACON_FRAMECTRL_MESH_MSK    (0x0080)
#define XMIBEACON_FRAMECTRL_REGISTR_MSK (0x0100)
#define XMIBEACON_FRAMECTRL_SOLICIT_MSK (0x0200)

bool xMiBeaconDecrypt(const uint8_t *bindKey, const uint8_t *macReverse, const uint8_t *payload, size_t totalLength,
    size_t encryptedDataOffset, uint8_t *outPlaintext, size_t &outLen);
bool xMiBeaconGetData(const uint8_t *serviceData, const size_t serviceDataLength, const uint8_t *bindKey, const uint8_t *inputMacRev,
    uint8_t *buffer, const uint8_t *&rDataPtrOut, size_t &rDataLenOut);
bool xMiIsServiceDataEncrypted(const uint8_t *serviceData);
