#include "si_parser.h"

const static char *TAG = "SI PARSER";

SIRecord parseSIdata(const uint8_t *data, size_t data_len)
{
  uint16_t si_stationnumber = 0;
  uint32_t si_cardnumber = 0;
  uint8_t si_weeknumrelative = 0;
  uint8_t si_weekday = 0;   // 0=sunday, 1=monday, 2=tuesday, 3=wednesday, 4=thursday, 5=friday, 6=saturday
  uint8_t si_fullday = 0;   // 0=am, 1=pm
  uint16_t si_h12timer = 0; // 12h timer in seconds

  // SIdoc CN1, CN0 2 bytes stations code number 1...999
  si_stationnumber = data[4] << 8 | data[5];

  // SIdoc SN3...SN0 4 bytes SI-Card number
  si_cardnumber = data[6] << 24 | data[7] << 16 | data[8] << 8 | data[9];

  // SIdoc TD 1 byte day-of-week/half day
  // bit5...bit4 4 week counter relative
  si_weeknumrelative = (data[10] & 0b00110000) >> 4;
  // bit3...bit1 day of week
  si_weekday = (data[10] & 0b00001110) >> 1;
  // bit0 24h counter (0-am, 1-pm)
  si_fullday = data[10] & 0b00000001;

  // SIdoc TH...TL 2 bytes 12h timer, binary
  si_h12timer = data[11] << 8 | data[12];

  // TSS 1 byte sub second values 1/256 sec
  // unused
  // si_subsec = data[13];

  SIRecord record;
  record.cardNumber = si_cardnumber;
  record.stationNumber = si_stationnumber;
  char time_buffer[9];
  snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d:%02d",
           (si_h12timer / 3600) + (si_fullday ? 12 : 0),
           (si_h12timer % 3600) / 60,
           si_h12timer % 60);
  record.time = std::string(time_buffer);

  return record;
}

SIRecord getTestSIRecord()
{
  SIRecord record = {0, 123456, 111, "12:22:20"};
  return record;
}

void dumpSiData(uint16_t si_stationnumber,
                uint32_t si_cardnumber,
                uint8_t si_weeknumrelative,
                uint8_t si_weekday,
                uint8_t si_fullday,
                uint16_t si_h12timer)
{

  std::string day_name = "";
  switch (si_weekday)
  {
  case 0:
    day_name = "Sunday";
    break;
  case 1:
    day_name = "Monday";
    break;
  case 2:
    day_name = "Tuesday";
    break;
  case 3:
    day_name = "Wednesday";
    break;
  case 4:
    day_name = "Thursday";
    break;
  case 5:
    day_name = "Friday";
    break;
  case 6:
    day_name = "Saturday";
    break;
  }

  ESP_LOGI(TAG, "station number: %d", si_stationnumber);
  ESP_LOGI(TAG, "card number: %d", si_cardnumber);
  ESP_LOGI(TAG, "week number relative: %d", si_weeknumrelative);
  ESP_LOGI(TAG, "week day: %s", day_name.c_str());
  ESP_LOGI(TAG, "24h time: %d:%d:%d", (si_h12timer / 3600) + (si_fullday ? 12 : 0), (si_h12timer % 3600) / 60, si_h12timer % 60);
}