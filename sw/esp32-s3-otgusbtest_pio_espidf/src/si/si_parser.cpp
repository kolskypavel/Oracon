#include "si_parser.h"

const static char *TAG = "si_parser";

uint16_t si_stationnumber = 0;
uint32_t si_cardnumber = 0;
uint8_t si_weeknumrelative = 0;
uint8_t si_weekday = 0;
uint8_t si_fullday = 0;
uint16_t si_h12timer = 0;

void si_parse(const uint8_t *data, size_t data_len)
{
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
  //si_subsec = data[13];
}

void si_clear_data(){
  si_stationnumber = 0;
  si_cardnumber = 0;
  si_weeknumrelative = 0;
  si_weekday = 0;
  si_fullday = 0;
  si_h12timer = 0;
  //si_subsec = 0;
}

void si_load_dummy_data(){
  si_stationnumber = 500;
  si_cardnumber = 2075683;
  si_weeknumrelative = 0;
  si_weekday = 1; //0=sunday, 1=monday, 2=tuesday, 3=wednesday, 4=thursday, 5=friday, 6=saturday
  si_fullday = 1; // 0=am, 1=pm
  si_h12timer = 42267; // 12h timer in seconds
  //si_subsec = 0;
}

std::string si_jsonify(){
  std::string json = "{";
  json += "\"station_number\":" + std::to_string(si_stationnumber) + ",";
  json += "\"card_number\":" + std::to_string(si_cardnumber) + ",";
  json += "\"week_number_relative\":" + std::to_string(si_weeknumrelative) + ",";
  json += "\"week_day\":" + std::to_string(si_weekday) + ",";
  json += "\"24h_time\":\"" + std::to_string((si_h12timer / 3600) + (si_fullday ? 12 : 0)) + ":" + std::to_string((si_h12timer % 3600) / 60) + ":" + std::to_string(si_h12timer % 60) + "\",";
  //json += "\"subsec\":" + std::to_string(si_subsec);
  json += "}";
  return json;
}

void si_dumpdata()
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