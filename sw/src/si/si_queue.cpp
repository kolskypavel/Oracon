#include "si_queue.h"

std::mutex mtx; // Prevent race conditions

File queueFile;    // Actual queue data
File metadataFile; // Start and end indexes

uint8_t readIndex = 0;
uint8_t writeIndex = 0;

bool initQueue()
{
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE("QUEUE", "Failed to mount file system");
        return false;
    }

    // DEBUG
    //  SPIFFS.remove(QUEUE_DATA_FILE_NAME);
    //  SPIFFS.remove(QUEUE_METADATA_FILE_NAME);

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
        metadataFile = SPIFFS.open(QUEUE_METADATA_FILE_NAME, "rb+");
        if (metadataFile.size() >= 2) // Ensure there is enough data to read
        {
            metadataFile.read((uint8_t *)&readIndex, sizeof(readIndex));
            metadataFile.read((uint8_t *)&writeIndex, sizeof(writeIndex));
            ESP_LOGI("QUEUE", "Indexes r %d w %d", readIndex, writeIndex);
        }
        else
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
        ESP_LOGE("QUEUE", "Failed to open files");
        return false;
    }

    return true;
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

    if (queueFile.write((uint8_t *)&record, sizeof(SIRecord)) != sizeof(SIRecord))
    {
        ESP_LOGE("QUEUE", "Failed to write record to queue file");
        return false;
    }
    queueFile.seek(0); // Workaround for non working flush

    writeIndex = ((writeIndex + 1) % PUNCH_QUEUE_SIZE);
    if (!backupIndexes())
    {
        ESP_LOGE("QUEUE", "Failed to backup indexes after enqueue");
        return false;
    }

    return true;
}

void receiveRecords(SIRecord *records, uint8_t &out)
{
    std::unique_lock<std::mutex> lock(mtx);
    out = 0;
    uint8_t tmpRead = readIndex;

    while (out < PUNCH_BUFFER_SIZE && tmpRead != writeIndex)
    {
        if (!queueFile.seek(tmpRead * sizeof(SIRecord)))
        {
            ESP_LOGE("QUEUE", "Failed to seek to read position in queue file");
            return;
        }

        if (queueFile.read((uint8_t *)&records[out], sizeof(SIRecord)) != sizeof(SIRecord))
        {
            ESP_LOGE("QUEUE", "Failed to read record from queue file");
            return;
        }

        out++;
        tmpRead = (tmpRead + 1) % PUNCH_QUEUE_SIZE;
    }

#ifdef TEST_QUEUE_VERBOSE
    ESP_LOGI("QUEUE", "Read %d punches", out);
#endif
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
    ESP_LOGI("QUEUE", "Indexes r %d w %d", readIndex, writeIndex);
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

    return true;
}