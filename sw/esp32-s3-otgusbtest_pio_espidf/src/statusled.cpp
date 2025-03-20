#include <Arduino.h>
#include <FastLED.h>

#include "statusled.hpp"  

#include <inttypes.h>
	
const static char *TAG = "statusled";

StatusLED::StatusLED(int NUMLEDS){ 
	FastLED.addLeds<WS2812B, 48, GRB>(this->leds, NUMLEDS);
  	//FastLED.setBrightness(25);
}

StatusLED::StatusLED(){};


void StatusLED::show(){
	//if(this->led_type == NO_HW_LED) return;

	if(this->led_enabled == false){
		setRGB(0, 0, 0);
		return;
	}

	//reset indicate state
	if(this->indicate_until != 0 && esp_timer_get_time() >= this->indicate_until){
		this->r = this->r_old;
		this->g = this->g_old;
		this->b = this->b_old;
		this->mode = this->mode_old;
		this->freq = this->freq_old;
		this->indicate_until = 0;
		calc_offset();
		ESP_LOGI(TAG, "indicate mode reset");
	}

	double coef;// = calc_sin(this->offset_ms, 0);

	switch(mode){
		case SOLID:
			setRGB(this->r, this->g, this->b);
			return;

		case FLASH:
			coef = calc_sin(this->offset_ms, -PI*0.5);
			if(coef < 0.5){
				setRGB(this->r, this->g, this->b);
			}else{
				setRGB(0, 0, 0);
			}
			return;

		case PULSE:
			coef = calc_sin(this->offset_ms, 0);
			setRGB(coef*this->r, coef*this->g, coef*this->b);
			return;
	}

}

void StatusLED::setEnabled(bool enabled){
	this->led_enabled = enabled;
}

void StatusLED::setColor(int r, int g, int b){
	this->r = r;
	this->g = g;
	this->b = b;
}

void StatusLED::setSolid(){
	this->mode = SOLID;
}

void StatusLED::setFlash(double freq){
	this->mode = FLASH;
	this->freq = freq;
	//calc_offset();
}

void StatusLED::setPulse(double freq){
	this->mode = PULSE;
	this->freq = freq;
	//calc_offset();
}

void StatusLED::indicate(int r, int g, int b, MODE mode, double freq, int duration_ms){
	//save the old state only if no other indication is already happening
	if(this->indicate_until == 0){
		this->r_old = this->r;
		this->g_old = this->g;
		this->b_old = this->b;
		this->mode_old = this->mode;
		this->freq_old = this->freq;	
	}

	//sets indicate state as current
	this->r = r;
	this->g = g;
	this->b = b;
	this->mode = mode;
	this->freq = freq;	
	
	//calculate end time of indication
	this->indicate_until = esp_timer_get_time() + MILLIS_TO_MICROS(duration_ms);
	ESP_LOGI(TAG, "current time: %" PRId64 ", until: %" PRId64, esp_timer_get_time(), this->indicate_until);
	calc_offset();	
}

double StatusLED::calc_sin(int offset_ms, double offset_rad){
	int period_ms = 1000 / this->freq;
  	double d = (double)((millis()+offset_ms)%period_ms) / period_ms;
	return (sin(d*2*PI + offset_rad)+1) / 2;
}

void StatusLED::calc_offset(){
	int offset = 0;

	//requiring the sin value to be zero might not be a great idea, might get stuck, did not happen yet
	while(calc_sin(offset, 0) > 0) offset += 1;
	this->offset_ms = offset;
	ESP_LOGD(TAG, "offset set: %u; sin val with offset: %f", this->offset_ms, calc_sin(this->offset_ms, 0));
}

void StatusLED::setRGB(int r, int g, int b){
	/*this->leds[0] = CRGB(r, g, b);
	FastLED.show();*/
	leds[0] = CRGB(0, 0, 255);
	FastLED.show();
	ESP_LOGI(TAG, "show run %u %u %u", r, g, b);
}
