#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <Arduino.h>
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct SIRecord {
    uint8_t cardNumber;
    uint8_t stationNumber;
    std::string time;
  };
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void si_parse(const uint8_t *data, size_t data_len);
void si_clear_data();
void si_load_dummy_data();
void si_dumpdata();

/**********************+
 *      MACROS
 **********************/