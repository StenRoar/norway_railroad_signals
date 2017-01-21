/*
 * signals.h
 *
 *  Created on: 28. nov. 2016
 *      Author: StenRoar
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_
#include "bulbs.h"

class Signal {
public:
	Signal(){}
	virtual ~Signal() {}
	virtual void Update(const unsigned long current_time)=0;
};

class TallShift : Signal {
public:
	enum class Images{
		OFF,
		ON
	};
	TallShift(unsigned int pin_no) : Signal(), m_bulb(pin_no) {}

	void SetImage(Images img) {
			switch(img) {
			case Images::ON:
				m_bulb.Enable();
				break;
			case Images::OFF:
			default:
				m_bulb.Disable();
			}
		}
	void Update(const unsigned long current_time) override {};


private:
	Bulb m_bulb;
};

class DistantSignal : Signal {
public:
	enum class Images{
			NONE,
			GREEN,
			ORANGE,
			BOTH
		};

	DistantSignal(unsigned int green_pin, unsigned int orange_pin) : Signal(),
	m_green(green_pin),
	m_orange(orange_pin)
	{}

	virtual ~DistantSignal() {}

	void SetImage(Images img) {
		switch(img) {
		case Images::GREEN:
			m_green.Enable();
			m_orange.Disable();
			break;
		case Images::ORANGE:
					m_green.Disable();
					m_orange.Enable();
					break;
		case Images::BOTH:
					m_green.Enable();
					m_orange.Enable();
					break;
		case Images::NONE:
		default:
			m_green.Disable();
			m_orange.Disable();
		}
	}

	void Update(const unsigned long current_time) override {
		m_green.Update(current_time);
		m_orange.Update(current_time);
	}


private:
	FlashingBulb    m_green; ///< The green bulb
	FlashingBulb 	m_orange; ///< The orange bulb
};

class MainInSignal : Signal {
public:
	enum class Images{
			GREEN_STRAIGHT,
			RED,
			GREEN_DEVIATE
		};

	MainInSignal(unsigned int top_green_pin, unsigned int center_red_pin,
			unsigned int bottom_green_pin, unsigned int distant_green_pin,
			unsigned int distant_orange_pin) : Signal(),
	m_top_green(top_green_pin),
	m_center_red(center_red_pin),
	m_bottom_green(bottom_green_pin),
	m_distant(distant_green_pin, distant_orange_pin),
	m_image(Images::RED){
		SetImage(m_image);
	}

	virtual ~MainInSignal() {}

	void SetImage(Images img) {
		m_image = img;
		switch(img) {
		case Images::GREEN_STRAIGHT:
			m_top_green.Enable();
			m_center_red.Disable();
			m_bottom_green.Enable();
			m_distant.SetImage(DistantSignal::Images::GREEN);
			break;
		case Images::RED:
					m_top_green.Disable();
					m_center_red.Enable();
					m_bottom_green.Disable();
					m_distant.SetImage(DistantSignal::Images::ORANGE);
					break;
		case Images::GREEN_DEVIATE:
					m_top_green.Enable();
					m_center_red.Disable();
					m_bottom_green.Disable();
					m_distant.SetImage(DistantSignal::Images::BOTH);
					break;
		default:
			m_top_green.Disable();
			m_center_red.Disable();
			m_bottom_green.Disable();
			m_distant.SetImage(DistantSignal::Images::NONE);
		}
	}

	void Update(const unsigned long current_time) override {
		m_center_red.Update(current_time);
		m_distant.Update(current_time);
	}

	MainInSignal::Images GetImage() {return m_image;}


private:
	Bulb 			m_top_green; 	///< The bulb at the top
	FlashingBulb 	m_center_red; 	///< The center red bulb
	Bulb 			m_bottom_green; ///< The bottom green bulb
	DistantSignal   m_distant; 		///< The distant signal
	Images 			m_image;		///< Stored image state

};

class MainOutSignal : Signal {
public:
	enum class Images{
			GREEN_STRAIGHT,
			RED,
			GREEN_DEVIATE
		};

	MainOutSignal(unsigned int s_top_green_pin,
			unsigned int s_center_red_pin,
			unsigned int s_bottom_green_pin,
			unsigned int d_green_pin,
			unsigned int d_red_pin,
			unsigned int distant_green_pin,
			unsigned int distant_orange_pin) : Signal(),
	m_s_top_green(s_top_green_pin),
	m_s_center_red(s_center_red_pin),
	m_s_bottom_green(s_bottom_green_pin),
	m_d_green(d_green_pin),
	m_d_red(d_red_pin),
	m_distant(distant_green_pin, distant_orange_pin){}

	virtual ~MainOutSignal() {}

	void SetImage(Images img, MainInSignal::Images main_in_image) {
		switch(img) {
		case Images::GREEN_STRAIGHT:
			m_s_top_green.Enable();
			m_s_center_red.Disable();
			m_s_bottom_green.Enable();
			m_d_green.Disable();
			m_d_red.Enable();
			if(main_in_image == MainInSignal::Images::RED)
				m_distant.SetImage(DistantSignal::Images::NONE);
			else if(main_in_image == MainInSignal::Images::GREEN_DEVIATE)
				m_distant.SetImage(DistantSignal::Images::ORANGE);
			else
				m_distant.SetImage(DistantSignal::Images::GREEN);
			break;
		case Images::RED:
			m_s_top_green.Disable();
			m_s_center_red.Enable();
			m_s_bottom_green.Disable();
			m_d_green.Disable();
			m_d_red.Enable();
			if(main_in_image == MainInSignal::Images::RED)
				m_distant.SetImage(DistantSignal::Images::NONE);
			else
				m_distant.SetImage(DistantSignal::Images::ORANGE);
			break;
		case Images::GREEN_DEVIATE:
					m_s_top_green.Disable();
					m_s_center_red.Enable();
					m_s_bottom_green.Disable();
					m_d_green.Enable();
					m_d_red.Disable();
					if(main_in_image == MainInSignal::Images::RED)
						m_distant.SetImage(DistantSignal::Images::NONE);
					else if(main_in_image == MainInSignal::Images::GREEN_STRAIGHT)
						m_distant.SetImage(DistantSignal::Images::ORANGE);
					else
						m_distant.SetImage(DistantSignal::Images::GREEN);
					break;
		default:
			m_s_top_green.Disable();
			m_s_center_red.Disable();
			m_s_bottom_green.Disable();
			m_d_green.Disable();
			m_d_red.Disable();
		}
	}

	void Update(const unsigned long current_time) override {
			m_distant.Update(current_time);
	}

private:
	Bulb 			m_s_top_green; 		///< Signal on straight track. The bulb at the top
	Bulb 			m_s_center_red; 	///< Signal on straight track. The center red bulb
	Bulb 			m_s_bottom_green; 	///< Signal on straight track.
	Bulb 			m_d_green; 			///< Signal on deviating track. Green bulbThe bottom green bulb
	Bulb 			m_d_red; 			///< Signal on deviating track. Red bulb
	DistantSignal   m_distant; 			///< The distant signal


};

struct ImagesCollection{
	MainInSignal::Images a_in_image;
	MainInSignal::Images b_in_image;
	MainOutSignal::Images a_out_image;
	MainOutSignal::Images b_out_image;
	TallShift::Images a_shift_image;
	TallShift::Images b_shift_image;
};


#endif /* SIGNALS_H_ */
