#include "statusled.h"
#include "Arduino.h"
#include "esp32-hal-ledc.h"
#include "defines.h"
#include <inttypes.h>

const static char *TAG = "statusled";

StatusLED::StatusLED(int pin_r, int channel_r, int pin_g, int channel_g, int pin_b, int channel_b, LED_TYPE led_type)
{
	this->led_type = led_type;

	switch (this->led_type)
	{
	case NO_HW_LED:
		ESP_LOGI(TAG, "status led DISABLED because of NO_HW_LED");
		break;

	case RGB_COMMON_ANODE:
	case RGB_COMMON_CATHODE:
		this->channel_r = channel_r;
		this->channel_g = channel_g;
		this->channel_b = channel_b;
		pinMode(pin_r, OUTPUT);
		pinMode(pin_g, OUTPUT);
		pinMode(pin_b, OUTPUT);
		ledcSetup(this->channel_r, 2000, 8); // channel 1, 5000Hz, 8bit PWM
		ledcSetup(this->channel_g, 2000, 8); // channel 2, 5000Hz, 8bit PWM
		ledcSetup(this->channel_b, 2000, 8); // channel 3, 5000Hz, 8bit PWM
		ledcAttachPin(pin_r, this->channel_r);
		ledcAttachPin(pin_g, this->channel_g);
		ledcAttachPin(pin_b, this->channel_b);
		ESP_LOGI(TAG, "set RGB");
		break;

	case SINGLE_ANODE:
	case SINGLE_CATHODE:
		pinMode(pin_r, OUTPUT);
		ledcSetup(3, 5000, 8); // channel 4, 5000Hz, 8bit PWM
		ledcAttachPin(pin_r, 3);
		ESP_LOGI(TAG, "set SINGLE");
		break;
	}
}

StatusLED::StatusLED() {};

void StatusLED::show()
{
	if (this->led_type == NO_HW_LED)
		return;

	if (this->led_enabled == false)
	{
		setRGB(0, 0, 0);
		return;
	}

	// reset indicate state
	if (this->indicate_until != 0 && esp_timer_get_time() >= this->indicate_until)
	{
		this->r = this->r_old;
		this->g = this->g_old;
		this->b = this->b_old;
		this->mode = this->mode_old;
		this->freq = this->freq_old;
		this->indicate_until = 0;
		calc_offset();
		ESP_LOGI(TAG, "indicate mode reset");
	}

	double coef; // = calc_sin(this->offset_ms, 0);

	switch (mode)
	{
	case SOLID:
		setRGB(this->r, this->g, this->b);
		return;

	case FLASH:
		coef = calc_sin(this->offset_ms, -PI * 0.5);
		if (coef < 0.5)
		{
			setRGB(this->r, this->g, this->b);
		}
		else
		{
			setRGB(0, 0, 0);
		}
		return;

	case PULSE:
		coef = calc_sin(this->offset_ms, 0);
		setRGB(coef * this->r, coef * this->g, coef * this->b);
		return;
	}
}

void StatusLED::setEnabled(bool enabled)
{
	this->led_enabled = enabled;
}

void StatusLED::setColor(int r, int g, int b)
{
	this->r = r;
	this->g = g;
	this->b = b;
}

void StatusLED::setColorPreset(StatusLED::COLOR_PRESET preset)
{
	switch (preset)
	{
	case COLOR_PRESET::GREEN:
		setColor(0, 128, 0);
		break;

	case COLOR_PRESET::ORANGE:
		setColor(255, 165, 0);
		break;

	case COLOR_PRESET::RED:
		setColor(255, 0, 0);
		break;

	case COLOR_PRESET::BLUE:
		setColor(0, 0, 255);
		break;
	default:
		break;
	}
}

void StatusLED::setMode(MODE mode, double freq)
{
	switch (mode)
	{
	case SOLID:
		this->mode = SOLID;
		break;

	case FLASH:
		this->mode = FLASH;
		this->freq = freq;
		break;

	case PULSE:
		this->mode = PULSE;
		this->freq = freq;
		break;

	default:
		break;
	}
}

void StatusLED::indicate(int r, int g, int b, MODE mode, double freq, int duration_ms)
{
	// save the old state only if no other indication is already happening
	if (this->indicate_until == 0)
	{
		this->r_old = this->r;
		this->g_old = this->g;
		this->b_old = this->b;
		this->mode_old = this->mode;
		this->freq_old = this->freq;
	}

	// sets indicate state as current
	this->r = r;
	this->g = g;
	this->b = b;
	this->mode = mode;
	this->freq = freq;

	// calculate end time of indication
	this->indicate_until = esp_timer_get_time() + MILLIS_TO_MICROS(duration_ms);
	ESP_LOGI(TAG, "current time: %" PRId64 ", until: %" PRId64, esp_timer_get_time(), this->indicate_until);
	calc_offset();
}

double StatusLED::calc_sin(int offset_ms, double offset_rad)
{
	int period_ms = 1000 / this->freq;
	double d = (double)((millis() + offset_ms) % period_ms) / period_ms;
	return (sin(d * 2 * PI + offset_rad) + 1) / 2;
}

void StatusLED::calc_offset()
{
	int offset = 0;

	// requiring the sin value to be zero might not be a great idea, might get stuck, did not happen yet
	while (calc_sin(offset, 0) > 0)
		offset += 1;
	this->offset_ms = offset;
	ESP_LOGD(TAG, "offset set: %u; sin val with offset: %f", this->offset_ms, calc_sin(this->offset_ms, 0));
}

void StatusLED::setRGB(int r, int g, int b)
{
	switch (this->led_type)
	{
	case RGB_COMMON_ANODE:
		ledcWrite(this->channel_r, (255 - r) * 0.5);
		ledcWrite(this->channel_g, (255 - g) * 0.5);
		ledcWrite(this->channel_b, (255 - b) * 0.5);
		break;

	case RGB_COMMON_CATHODE:
		ledcWrite(this->channel_r, r * 0.2);
		ledcWrite(this->channel_g, g * 0.2);
		ledcWrite(this->channel_b, b * 0.2);
		break;

	case SINGLE_ANODE:
		ledcWrite(3, r);
		break;

	case SINGLE_CATHODE:
		ledcWrite(3, 255 - r);
		break;

	case NO_HW_LED:
		// do nothing
		break;
	}
}
