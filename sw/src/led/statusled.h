#ifndef STATUSLED_H
#define STATUSLED_H
#pragma once

#include "stdint.h"

/**
 * This class takes care of interacting with the user on the module side with an RGB led.
 * @source David Rothbauer + https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html
 */

class StatusLED
{
public:
	enum LED_TYPE
	{
		NO_HW_LED,
		SINGLE_ANODE,
		SINGLE_CATHODE,
		RGB_COMMON_ANODE,
		RGB_COMMON_CATHODE
	};
	enum MODE
	{
		SOLID,
		FLASH,
		PULSE
	};

	enum COLOR_PRESET
	{
		RED,
		GREEN,
		BLUE,
		ORANGE
	};

	StatusLED(int pin_r, int channel_r, int pin_g, int channel_g, int pin_b, int channel_b, LED_TYPE led_type);
	StatusLED();

	void setEnabled(bool enabled);
	void setColor(int r, int g, int b);
	void setColorPreset(COLOR_PRESET preset);
	void setMode(MODE mode, double freq);
	void indicate(int r, int g, int b, MODE mode, double freq, int duration_ms);
	void show();

private:
	LED_TYPE led_type;
	MODE mode, mode_old;
	int channel_r, channel_g, channel_b;

	bool led_enabled = true;
	int r = 0, g = 0, b = 0, r_old = 0, g_old = 0, b_old = 0, offset_ms = 0;
	double freq = 0, freq_old = 0;
	uint64_t indicate_until = 0;

	void setRGB(int r, int g, int b);
	void calc_offset();
	double calc_sin(int offset_ms, double offset_rad);
};
#endif