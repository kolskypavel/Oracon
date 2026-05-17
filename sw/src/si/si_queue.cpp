#include "si_queue.h"

std::mutex mtx; // Prevent race conditions

File queueFile;    // Actual queue data
File metadataFile; // Start and end indexes

uint8_t readIndex = 0;
uint8_t writeIndex = 0;

void initQueue()
{
    if (!SPIFFS.begin(true))
    {
        throw std::runtime_error("QUEUE: Failed to mount file system");
    }

    // DEBUG
    //  SPIFFS.format();

    if (SPIFFS.exists(QUEUE_DATA_FILE_NAME))
    {
        queueFile = SPIFFS.open(QUEUE_DATA_FILE_NAME, "rb+"); // Read/write, keep content
    }
    else
    {
        queueFile = SPIFFS.open(QUEUE_DATA_FILE_NAME, "wb+"); // Create new file
    }

    if (SPIFFS.exists(QUEUE_METADATA_FILE_NAME))
    {
        bool valid = true;
        metadataFile = SPIFFS.open(QUEUE_METADATA_FILE_NAME, "rb+");

        // Ensure there is enough data to read
        if (metadataFile.size() >= 2)
        {
            if (!metadataFile.seek(0))
            {
                {
                    ESP_LOGE("QUEUE", "Failed to seek to start position in metadata file");
                    valid = false;
                }
            }
            if (metadataFile.read((uint8_t *)&readIndex, sizeof(readIndex)) != 1 ||
                metadataFile.read((uint8_t *)&writeIndex, sizeof(writeIndex)) != 1)
            {
                ESP_LOGE("QUEUE", "Failed to read values from metadata file");
                valid = false;
            };

            ESP_LOGI("QUEUE", "Indexes r %d w %d", readIndex, writeIndex);

            // Check if indexes are not corrupted
            if (writeIndex > PUNCH_QUEUE_SIZE || readIndex > writeIndex)
            {
                valid = false;
            }
        }
        if (!valid)
        {
            ESP_LOGW("QUEUE", "Metadata file is corrupted or empty, resetting indexes");
            readIndex = 0;
            writeIndex = 0;
        }
    }
    else
    {
        metadataFile = SPIFFS.open(QUEUE_METADATA_FILE_NAME, "wb+");
    }

    if (!queueFile || !metadataFile)
    {
        throw std::runtime_error("QUEUE: Failed to open files");
    }
}

bool enqueueRecord(const SIRecord &record)
{
    std::unique_lock<std::mutex> lock(mtx);

#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "File size %d, record size %d, write %d", queueFile.size(), sizeof(SIRecord), writeIndex);
#endif
    // Check if queue is full
    if ((writeIndex + 1) % PUNCH_QUEUE_SIZE == readIndex)
    {
        ESP_LOGW("QUEUE", "Queue is full, cannot enqueue");
        return false;
    }

    if (!queueFile.seek(writeIndex * sizeof(SIRecord)))
    {
        ESP_LOGE("QUEUE", "Failed to seek to write position in queue file");
        return false;
    }
#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Write - successfuly seeked to pos %d", writeIndex * sizeof(SIRecord));
#endif

    if (queueFile.write((uint8_t *)&record, sizeof(SIRecord)) != sizeof(SIRecord))
    {
        ESP_LOGE("QUEUE", "Failed to write record to queue file");
        return false;
    }

    queueFile.flush();
    queueFile.seek(0); // Workaround for non working flush

#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Write - successfuly flushed to file");
#endif

    writeIndex = ((writeIndex + 1) % PUNCH_QUEUE_SIZE);
    if (!backupIndexes())
    {
        ESP_LOGE("QUEUE", "Failed to backup indexes after enqueue");
        return false;
    }

    return true;
}

uint8_t receiveRecords(SIRecord *records, uint8_t &consumed)
{
    std::unique_lock<std::mutex> lock(mtx);
    SIRecord tmpRecord;
    uint8_t tmpRead = readIndex;
    uint8_t validCount = 0;
    consumed = 0;

    while (validCount < PUNCH_BUFFER_SIZE && tmpRead != writeIndex)
    {
        if (!queueFile.seek(tmpRead * sizeof(SIRecord)))
        {
#ifdef TEST_QUEUE_VERBOSE
            ESP_LOGE("QUEUE", "Failed to seek to read position in queue file");
#endif
            break;
        }

        if (queueFile.read((uint8_t *)&tmpRecord, sizeof(SIRecord)) != sizeof(SIRecord))
        {
#ifdef TEST_QUEUE_VERBOSE
            ESP_LOGE("QUEUE", "Failed to read record from queue file");
#endif
            break;
        }

        consumed++;
        tmpRead = (tmpRead + 1) % PUNCH_QUEUE_SIZE;

        // Validate record
        if (validateRecord(tmpRecord))
        {
            records[validCount] = tmpRecord;
            validCount++;
        }
#ifdef TEST_QUEUE_VERBOSE
        else
        {
            ESP_LOGE("QUEUE", "Failed to validate record");
        }
#endif
    }

#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Read %d punches, consumed %d queue entries", validCount, consumed);
#endif

    return validCount;
}

bool removeRecords(uint8_t size)
{
    std::unique_lock<std::mutex> lock(mtx);
    // Check if the queue has enough elements to pop
    uint8_t available = (writeIndex >= readIndex) ? (writeIndex - readIndex) : (PUNCH_QUEUE_SIZE - readIndex + writeIndex);
    if (size > available)
    {
        return false;
    }

    // Update the readIndex to effectively "pop" the elements
    readIndex = (readIndex + size) % PUNCH_QUEUE_SIZE;
    return backupIndexes();
}

bool backupIndexes()
{
#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Saving indexes");
#endif
    if (!metadataFile.seek(0))
    {
        ESP_LOGE("QUEUE", "Failed to seek metadata file");
        return false;
    }

    if (metadataFile.write(readIndex) != 1)
    {
        ESP_LOGE("QUEUE", "Failed to write readIndex to metadata file");
        return false;
    }

    if (metadataFile.write(writeIndex) != 1)
    {
        ESP_LOGE("QUEUE", "Failed to write writeIndex to metadata file");
        return false;
    }

    metadataFile.seek(0); // Workaround for non working flush

#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Indexes r %d w %d saved to a file", readIndex, writeIndex);
#endif
    return true;
}