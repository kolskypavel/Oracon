#include "SPIFFS.h"
#include "FS.h"
#include "defines.h"
#include "si_parser.h"
#include <mutex>

#pragma once

// Inits the filesystem and queue, returns number of unsend records
void initQueue();

//Puts a new record in the queue
bool enqueueRecord(const SIRecord &record);

// Receives at most PUNCH_BUFFER_SIZE valid records.
// The reference parameter returns how many queue entries were consumed while scanning.
uint8_t receiveRecords(SIRecord *records, uint8_t &consumed);

// Removes requested number of records from the queue, if possible
bool removeRecords(uint8_t size);

//Backs up the read and write indexes to a file
bool backupIndexes();