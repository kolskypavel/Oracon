#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <Arduino.h>
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/
#define BYTE_STX 0x02
#define BYTE_PUNCH_DATA 0xD3
#define BYTE_ETX 0x03

/**********************
 *      TYPEDEFS
 **********************/
struct SIRecord
{
  uint32_t cardNumber = 0;
  uint16_t stationNumber = 0;
  std::string time;
};
/**********************
 * GLOBAL PROTOTYPES
 **********************/
SIRecord parseSIdata(const uint8_t *data, size_t data_len);

// Returns an example SI record for testing purposes
SIRecord getTestSIRecord();

// Logs raw data to serial
void dumpSiData(uint16_t si_stationnumber,
                uint32_t si_cardnumber,
                uint8_t si_weeknumrelative,
                uint8_t si_weekday,
                uint8_t si_fullday,
                uint16_t si_h12timer);

// Logs record data to serial
void dumpSIRecord(const SIRecord &record);

/**********************+
 *      MACROS
 **********************/