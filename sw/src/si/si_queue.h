#include "SPIFFS.h"
#include "FS.h"
#include "defines.h"
#include "si_parser.h"
#include <mutex>

#pragma once

// Inits the filesystem and queue
bool initQueue();

//Puts a new record in the queue
bool enqueueRecord(const SIRecord &record);

//Receives at most PUNCH_BUFFER_SIZE records, data remains in queue until pop() is called
void receiveRecords(SIRecord *records, uint8_t &out);

// Removes requested number of records from the queue, if possible
bool removeRecords(uint8_t size);

//Backs up the read and write indexes to a file
bool backupIndexes();