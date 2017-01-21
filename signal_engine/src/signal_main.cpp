/*
 * soft_blink.c
 *
 *  Created on: 25. okt. 2016
 *      Author: StenRoar
 */
#include "Arduino.h"
#include "signals.h"
#include "bulb_pindefs.h"
#include "states.h"

/** Global variables for the signals. Note that outbound signals are merged (2+3 light) */
MainInSignal main_in_A(A_IN_TOP_GREEN, A_IN_RED, A_IN_BOTTOM_GREEN, A_DISTANT_IN_GREEN, A_DISTANT_IN_ORANGE);
MainInSignal main_in_B(B_IN_TOP_GREEN, B_IN_RED, B_IN_BOTTOM_GREEN, B_DISTANT_IN_GREEN, B_DISTANT_IN_ORANGE);
MainOutSignal main_out_A(A_OUT_S_TOP_GREEN, A_OUT_S_RED, A_OUT_S_BOTTOM_GREEN, A_OUT_D_GREEN, A_OUT_D_RED, A_DISTANT_OUT_GREEN, A_DISTANT_OUT_ORANGE);
MainOutSignal main_out_B(B_OUT_S_TOP_GREEN, B_OUT_S_RED, B_OUT_S_BOTTOM_GREEN, B_OUT_D_GREEN, B_OUT_D_RED, B_DISTANT_OUT_GREEN, B_DISTANT_OUT_ORANGE);
TallShift shift_A(A_TALL_SHIFT);
TallShift shift_B(B_TALL_SHIFT);

/** Error LEDs - that flash at 2Hz when a dangerous situation is detected
 *  When a tall shift signal is lit, the error led at the respective end
 *  is flashing at 0.5Hz
 */
FlashingBulb error_led_a(A_ERROR_LED);
FlashingBulb error_led_b(B_ERROR_LED);

ImagesCollection images; 				///< The group of signal images to be shown
NormalState normal_state; 				///< The "normal" state (everything red)
CrossingState crossing_state; 			///< The crossing state. Two trains are to cross
PassState pass_state; 					///< Passing state. One train is passing the station.
InToStopState in_to_stop_state; 		///< In to stop state. A train is arriving the station, but will not depart.
OutState out_state;						///< Out state. No trains are arriving the station, but a train already present is leaving.
InToStopOutState in_to_stop_out_state;	///< In to stop, out state. A complex state. One train is arriving at one end, while a different train is leaving at the other end (other track)
ShiftingState shifting_state;			///< Shifting states. One or both tall shift lights are lit. All signals on the end where shift signal is lift, are red. On the other side, a train may leave. No trains may arrive at the station.

/** Global state handling variables */
State* all_states[7];
State* current_state = (State *) &normal_state;
LogicState logic_state;

/** Global constants */
const unsigned long logic_poll_interval = 500; //Poll switches each 0.5s. No point doing that each iteration...
int a_in_pin = 53; ///< Pin to read user request green signal IN end A
int a_out_pin = 51; ///< Pin to read user request green signal OUT end A
int a_dev_pin = 24; ///< Pin to read deviating outer turnout end A
int a_shift_pin = 49; ///< Pin to read user shift lock request end A

int b_in_pin = 50; ///< Pin to read user request green signal IN end B
int b_out_pin = 48; ///< Pin to read user request green signal OUT end B
int b_dev_pin = 22; ///< Pin to read deviating outer turnout end B
int b_shift_pin = 52; ///< Pin to read user shift lock request end B

/** Helper function to update all signals with the images for the current station state */
void set_images()
{
	current_state->GetImages(images);
	main_in_A.SetImage(images.a_in_image);
	main_in_B.SetImage(images.b_in_image);
	main_out_A.SetImage(images.a_out_image, main_in_B.GetImage());
	main_out_B.SetImage(images.b_out_image, main_in_A.GetImage());
	shift_A.SetImage(images.a_shift_image);
	shift_B.SetImage(images.b_shift_image);
}

/** Helper function to update signals (flashing) */
void update_signals(const unsigned long &current_time)
{
	main_in_A.Update(current_time);
	main_in_B.Update(current_time);
	main_out_A.Update(current_time);
	main_out_B.Update(current_time);
}

/** Function to check if something has changed on the logical input ports
 *  Please note that the logic is inverted due to using internal pull-ups
 *  (pins defined as INPUT_PULLUP)
 */
bool poll_logic_change() {
	LogicState c_logic_state;
	int a_in = digitalRead(a_in_pin);
	int a_out = digitalRead(a_out_pin);
	if(a_in==LOW)
		c_logic_state.switch_A = 1;
	if(a_out==LOW)
		c_logic_state.switch_A = -1;
	if(a_in==HIGH && a_out==HIGH)
		c_logic_state.switch_A = 0;

	int b_in = digitalRead(b_in_pin);
	int b_out = digitalRead(b_out_pin);
	if(b_in==LOW)
		c_logic_state.switch_B = -1;
	if(b_out==LOW)
		c_logic_state.switch_B = 1;
	if(b_in==HIGH && b_out==HIGH)
		c_logic_state.switch_B = 0;

	c_logic_state.turnout_A = (digitalRead(a_dev_pin)==LOW) ? true : false;
	c_logic_state.turnout_B = (digitalRead(b_dev_pin)==LOW) ? true : false;
	c_logic_state.shift_A = (digitalRead(a_shift_pin)==LOW) ? true : false;
	c_logic_state.shift_B = (digitalRead(b_shift_pin)==LOW) ? true : false;

	if(c_logic_state != logic_state) {
		logic_state = c_logic_state;
		return true;
	}

	return false;
}
/** Helper function to check which of the station states fits the logical mask
 *  derived from the switches position
 */
bool update_state() {
	//iterate over state array and find which is active
	for(unsigned int i = 0; i < 7; ++i) {
		if(all_states[i]->IsValid(logic_state)) {
			current_state = all_states[i];
			return true;
		}
	}
	//If no state is valid, default to normal state and return false
	current_state = (State *) &normal_state;
	return false;
}

/** Fill global state array with pointers to the different states. */
void initialize_states() {
	all_states[0] = (State *) &normal_state;
	all_states[1] = (State *) &pass_state;
	all_states[2] = (State *) &crossing_state;
	all_states[3] = (State *) &in_to_stop_state;
	all_states[4] = (State *) &out_state;
	all_states[5] = (State *) &in_to_stop_out_state;
	all_states[6] = (State *) &shifting_state;
}

/** Function that sets up all pin modes and sets the station in its normal state */
void setup(void)
{
	pinMode(a_in_pin, INPUT_PULLUP);
	pinMode(a_out_pin, INPUT_PULLUP);
	pinMode(a_dev_pin, INPUT_PULLUP);
	pinMode(a_shift_pin, INPUT_PULLUP);

	pinMode(b_in_pin, INPUT_PULLUP);
	pinMode(b_out_pin, INPUT_PULLUP);
	pinMode(b_dev_pin, INPUT_PULLUP);
	pinMode(b_shift_pin, INPUT_PULLUP);

	error_led_a.SetFadeupTime(0);
	error_led_a.SetFadedownTime(0);
	error_led_b.SetFadeupTime(0);
	error_led_b.SetFadedownTime(0);
	initialize_states();
	set_images();
}

/** Function to set error LED states */
void set_error_leds(const bool &error) {
	if(error) {
		error_led_a.Enable();
		if(logic_state.switch_A)
			error_led_a.SetFlashRate(2000);
		else
			error_led_a.SetFlashRate(500);

		error_led_b.Enable();
		if(logic_state.switch_B)
			error_led_b.SetFlashRate(2000);
		else
			error_led_b.SetFlashRate(500);
	} else {
		error_led_a.Disable();
		error_led_b.Disable();
	}
}

/** Main loop */
void loop(void)
{
	const unsigned long current_time = millis();
	if(((current_time%logic_poll_interval) == 0) && poll_logic_change())/*short circuit evaluation*/ {
		bool is_valid_state = update_state();
		set_error_leds(is_valid_state);
		set_images();
	}

	update_signals(current_time);
}



