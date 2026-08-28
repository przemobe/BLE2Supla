/*
 * SPDX-FileCopyrightText: 2026 Przemyslaw Bereski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XMiBeacon.hpp"
#include <mbedtls/ccm.h>
#include <esp_check.h>
#include <esp_log.h>
#include <cstring>

const uint16_t XMIBEACON_UUID = 0xfe95;
const uint8_t XMIBEACON_AUTHDATA = 0x11;

// References:
// https://iot.mi.com/v2/new/doc/embedded-dev/ble-sdk/function-dev/ble-mibeacon
// https://cdn.cnbj0.fds.api.mi-img.com/miio.files/commonfile_pdf_f119c8464d43526b48fb453f19f30192.pdf
// https://github.com/pvvx/ATC_MiThermometer/blob/master/InfoMijiaBLE/Mijia%20BLE%20MiBeacon%20protocol%20v5.md
// https://iot.mi.com/v2/new/doc/embedded-dev/ble-sdk/function-dev/ble-mibeacon
// https://github.com/Ernst79/bleparser/blob/3.7.3/package/bleparser/xiaomi.py

bool xMiBeaconDecrypt(const uint8_t *bindKey, const uint8_t *macReverse, const uint8_t *payload, size_t totalLength,
    size_t encryptedDataOffset, uint8_t *outPlaintext, size_t &outLen)
{
    // MiBeacon V4/V5 encrypted frame structure at the end:
    // [Encrypted Payload Data] + [3 Bytes Random Number] + [4 Bytes Message Integrity Check]
    if (totalLength <= encryptedDataOffset + XMIBEACON_RANDNUM_LEN + XMIBEACON_MIC_LEN)
    {
        return false;
    }

    size_t payloadLen = totalLength - encryptedDataOffset - (XMIBEACON_RANDNUM_LEN + XMIBEACON_MIC_LEN);
    const uint8_t* pEncData = &payload[encryptedDataOffset];
    const uint8_t* pRandNum = &payload[encryptedDataOffset + payloadLen];
    const uint8_t* pMIC = &payload[encryptedDataOffset + payloadLen + XMIBEACON_RANDNUM_LEN];

    // Construct the 12-byte CCM Nonce: [MAC reversed (6B)] + [Product ID (2B)] + [Frame Counter (4B)]
    uint8_t nonce[XMIBEACON_NONCE_LEN];
    memcpy(nonce, macReverse, 6);
    // Product ID
    nonce[6] = payload[XMIBEACON_PRODID_BYTE0];
    nonce[7] = payload[XMIBEACON_PRODID_BYTE1];
    // Frame Counter
    nonce[8] = payload[XMIBEACON_FRAMECTR_BYTE0];
    nonce[9] = pRandNum[0];
    nonce[10] = pRandNum[1];
    nonce[11] = pRandNum[2];

    // Initialize mbedtls CCM context
    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);

    int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, bindKey, XMIBEACON_BINDKEY_BIT_LEN);
    if (0 != ret)
    {
        mbedtls_ccm_free(&ctx);
        ESP_LOGE(XMIBEACON_TAG, "mbedtls_ccm_setkey failed, ret=%d", ret);
        return false;
    }

    // Perform authenticated decryption and integrity check
    ret = mbedtls_ccm_auth_decrypt(&ctx, payloadLen,
                                   nonce, XMIBEACON_NONCE_LEN,
                                   &XMIBEACON_AUTHDATA, XMIBEACON_AUTHDATA_LEN,
                                   pEncData, outPlaintext,
                                   pMIC, XMIBEACON_MIC_LEN);

    mbedtls_ccm_free(&ctx);

    if (0 != ret)
    {
        ESP_LOGE(XMIBEACON_TAG, "mbedtls_ccm_auth_decrypt failed, ret=%d", ret);
        return false; // Authentication fail (wrong key or corrupted message)
    }

    outLen = payloadLen;
    return true; // Success! MAC and payload are validated
}

bool xMiBeaconGetData(const uint8_t *serviceData, const size_t serviceDataLength, const uint8_t *bindKey, const uint8_t *inputMacRev,
    uint8_t *buffer, const uint8_t *&rDataPtrOut, size_t &rDataLenOut)
{
    const uint8_t *macReverse = inputMacRev;

    if (5 > serviceDataLength)
    {
        ESP_LOGE(XMIBEACON_TAG, "serviceData is too short, length=%d", serviceDataLength);
        return false;
    }

    // Extract Frame Control Bits
    const uint16_t frameControl = serviceData[XMIBEACON_FRAMECTRL_BYTE0] | (serviceData[XMIBEACON_FRAMECTRL_BYTE1] << 8);
    const uint8_t version = 0xF & (frameControl >> 12);

    if (5 != version)
    {
        ESP_LOGE(XMIBEACON_TAG, "Not supported protocol version=%d", version);
        return false;
    }

    if (!(frameControl & XMIBEACON_FRAMECTRL_OBJINC_MSK))
    {
        ESP_LOGE(XMIBEACON_TAG, "objectInclude=false!");
        return false;
    }

    // Establish the starting position of payload metadata
    size_t payloadDataOffset = 5;

    // MAC Include
    if (frameControl & XMIBEACON_FRAMECTRL_MACINC_MSK)
    {
        macReverse = &serviceData[payloadDataOffset];
        payloadDataOffset += XMIBEACON_MAC_LEN;
    }

    // Capability Include
    if (frameControl & XMIBEACON_FRAMECTRL_CAPINC_MSK)
    {
        payloadDataOffset += 1;
    }

    if (payloadDataOffset > serviceDataLength)
    {
        ESP_LOGE(XMIBEACON_TAG, "serviceData is too short, length=%d", serviceDataLength);
        return false;
    }

    // Is encrypted
    if (frameControl & XMIBEACON_FRAMECTRL_ISENCR_MSK)
    {
        if (nullptr == bindKey)
        {
            ESP_LOGW(XMIBEACON_TAG, "Encrypted serviceData - no input BIND KEY");
            return false;
        }
        if (nullptr == macReverse)
        {
            ESP_LOGW(XMIBEACON_TAG, "Encrypted serviceData - no input MAC");
            return false;
        }
        if (nullptr == buffer)
        {
            ESP_LOGE(XMIBEACON_TAG, "Encrypted serviceData - missing buffer for decryption");
            return false;
        }

        if (false == xMiBeaconDecrypt(bindKey, macReverse, serviceData, serviceDataLength, payloadDataOffset, buffer, rDataLenOut))
        {
            ESP_LOGE(XMIBEACON_TAG, "xMiBeaconDecrypt failed");
            return false;
        }

        // Set pointer directly to our freshly generated plaintext data
        rDataPtrOut = buffer;
    }
    else
    {
        // Process the remaining normal layout (minus the trailing 1-byte CRC found on legacy formats)
        rDataPtrOut = &serviceData[payloadDataOffset];
        rDataLenOut = serviceDataLength - payloadDataOffset - 1;
    }

    return true;
}

bool xMiIsServiceDataEncrypted(const uint8_t *serviceData)
{
    if (nullptr == serviceData)
    {
        return false;
    }
    const uint16_t frameControl = serviceData[XMIBEACON_FRAMECTRL_BYTE0] | (serviceData[XMIBEACON_FRAMECTRL_BYTE1] << 8);
    return (frameControl & XMIBEACON_FRAMECTRL_ISENCR_MSK);
}
