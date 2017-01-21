/* Signaldefs.h - Library for controlling single railroad signals
 * of the type used by the Norwegian State Railways (NSB).
 * Created by Sten Roar Snare, October 27, 2015.
 * Under development
*/
#ifndef SIGNAL_H
#define SIGNAL_H
#include "Arduino.h"

class Bulb
{
public:
	Bulb(int pin_no) : m_pin_no(pin_no),
	m_enabled(false),
	m_on_pwm_value(200),
	m_off_pwm_value(0)
	 {
		pinMode(pin_no, OUTPUT);
	}

	virtual ~Bulb() {}

	virtual void Enable() {
		digitalWrite(m_pin_no, m_on_pwm_value);
		m_enabled = true;
	}

	virtual void Disable() {
		digitalWrite(m_pin_no, m_off_pwm_value);
		m_enabled = false;
	}

protected:
	int m_pin_no; 					///< The pin number for the bulb
	bool m_enabled; 				///< Enable (light) the bulb
	unsigned int m_on_pwm_value; 	///< Pwm value for a lit bulb (intensity)
	unsigned int m_off_pwm_value;   ///< Pwm value for a dark bulb (normally 0)
};

class FlashingBulb : public Bulb
{
public:
	FlashingBulb(const int pin_no) : Bulb(pin_no),
			m_blink_interval(1000),
			m_fadeup_time(100),
			m_fadedown_time(200),
			m_on_percentage(60.0f) {
		m_fade_up_end = m_fadeup_time;
		m_fade_down_start = (int)(m_blink_interval*m_on_percentage/100.0f);
		m_fade_down_end = m_fade_down_start + m_fadedown_time;
		m_prev_brightness = 0;
	}

	void Update(const unsigned long current_time) {
		if(!m_enabled)
			return;

		unsigned long interval_time = current_time % m_blink_interval;
			unsigned int brightness;
			if(interval_time < m_fade_up_end) {
				float s = ((float) interval_time);
		                s /= m_fade_up_end;
				brightness = (int) ((1.0f - s)*m_off_pwm_value + s*m_on_pwm_value);
			} else if(interval_time >= m_fade_up_end && interval_time < m_fade_down_start) {
				brightness = m_on_pwm_value;
			} else if(interval_time >= m_fade_down_start && interval_time < m_fade_down_end) {
				float s = ((float) (interval_time-m_fade_down_start));
		                s /= m_fadedown_time;
				brightness = (int)((1.0f - s)*m_on_pwm_value + s*m_off_pwm_value);
			} else {
				brightness = m_off_pwm_value;
			}

			if(brightness != m_prev_brightness)
			{
				analogWrite(m_pin_no, brightness);
				m_prev_brightness = brightness;
			}
	}
	void SetFlashRate(const unsigned int &blink_interval) {
		m_blink_interval = blink_interval;
	}

	virtual void SetOnPwm(const unsigned int &on_pwm_value) {
		m_on_pwm_value = on_pwm_value;
	}

	virtual void SetFadeupTime(const unsigned int &fadeup_time) {
			m_fadeup_time = fadeup_time;
	}

	virtual void SetFadedownTime(const unsigned int &fadedown_time) {
				m_fadedown_time = fadedown_time;
	}

private:
	unsigned int m_blink_interval; 	//interval between flashes [ms]
	unsigned int m_fadeup_time;		//rise time [ms]
	unsigned int m_fadedown_time;   //fall time [ms]
	float m_on_percentage;    		//the percentage of the period the LED should stay lit

	unsigned int m_fade_up_end;
	unsigned int m_fade_down_start;
	unsigned int m_fade_down_end;
	unsigned int m_prev_brightness;

};

#endif
