#include "SPIFFS.h"
#include "defines.h"
#include "si_parser.h"
#include <mutex>

#pragma once

bool initQueue();

bool enqueue(const SIRecord &record);

void receiveRecords(SIRecord *records, uint8_t &out);

bool pop(uint8_t size);

bool backupIndexes();