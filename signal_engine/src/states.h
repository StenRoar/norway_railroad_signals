/*
 * states.h
 * Main class handling the state logic
 *
 *  Created on: 1. des. 2016
 *      Author: StenRoar
 */

#ifndef STATES_H_
#define STATES_H_

#include "signals.h"

struct LogicState{
	int switch_A;
	int switch_B;
	bool turnout_A;
	bool turnout_B;
	bool shift_A;
	bool shift_B;

	bool operator==(const LogicState & other) {
		if(switch_A == other.switch_A &&
				switch_B == other.switch_B &&
				turnout_A == other.turnout_A &&
				turnout_B == other.turnout_B &&
				shift_A == other.shift_A &&
				shift_B == other.shift_B)
			return true;
		else
			return false;
	}

	bool operator != (const LogicState & other) {
		return !(*this == other);
	}
};


/** Interface for all states */
class State {
public:
	State(){}
	virtual ~State() {}
	/** Checks if the current switches position corresponds to this state */
	virtual bool IsValid(const LogicState &logic_state)=0;

	/** Returns the complete set of signal images to deploy */
	virtual void GetImages(ImagesCollection &collection) = 0;
};

/** Normal state: All signals red. Shift lights dimmed */
class NormalState : public State {
public:
	NormalState(){}
	virtual ~NormalState() {}

	bool IsValid(const LogicState &ls) override {
		if(ls.switch_A==0 && ls.switch_B==0 && !ls.shift_A && !ls.shift_B)
			return true;
		else
			return false;
	}

	void GetImages(ImagesCollection &collection) override {
		collection.a_in_image = MainInSignal::Images::RED;
		collection.b_in_image = MainInSignal::Images::RED;
		collection.a_out_image = MainOutSignal::Images::RED;
		collection.b_out_image = MainOutSignal::Images::RED;
		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
	}
};

/** Passing state. */
class PassState : public State {
public:
	PassState(): State(), m_dir_is_ab(true), m_deviation(false){}
	virtual ~PassState() {}

	bool IsValid(const LogicState &ls) override {
		if((ls.switch_A==0) || (ls.switch_B==0) || ls.switch_A == ls.switch_B || ls.turnout_A != ls.turnout_B || ls.shift_A || ls.shift_B)
			return false;

		m_dir_is_ab = (ls.switch_A == 1);

		m_deviation = ls.turnout_A;;

		return true;
	}

	void GetImages(ImagesCollection &collection) override {
		if(m_dir_is_ab) {
			collection.a_in_image = m_deviation ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = m_deviation ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
		} else if(!m_dir_is_ab) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = m_deviation ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
			collection.b_in_image = m_deviation ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.b_out_image = MainOutSignal::Images::RED;
		}
		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
	}
private:
	bool m_dir_is_ab;
	bool m_deviation;
};

class CrossingState : public State {
public:
	CrossingState(): State(), m_deviation_a(false){}
	virtual ~CrossingState() {}

	bool IsValid(const LogicState &ls) override {
		if(ls.switch_A != 1 || ls.switch_B != 1 || ls.turnout_A == ls.turnout_B || ls.shift_A || ls.shift_B)
			return false;

		m_deviation_a = ls.turnout_A;

		return true;
	}

	void GetImages(ImagesCollection &collection) override {
		collection.a_in_image = m_deviation_a ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
		collection.b_in_image = m_deviation_a ? MainInSignal::Images::GREEN_STRAIGHT : MainInSignal::Images::GREEN_DEVIATE;

		collection.a_out_image = MainOutSignal::Images::RED;
		collection.b_out_image = MainOutSignal::Images::RED;
		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
	}
private:
	bool m_deviation_a; ///< True of crossing train in direction AB is over deviating turnout
};

class InToStopState : public State {
public:
	InToStopState(): State(), m_dir_is_ab(true), m_deviation(false){}
	virtual ~InToStopState() {}

	bool IsValid(const LogicState &ls) override {
		if(ls.shift_A || ls.shift_B)
			return false;


		if(ls.switch_A == 1 && ls.switch_B == 0) {
			m_deviation = ls.turnout_A;
			m_dir_is_ab = true;
			return true;
		}

		if(ls.switch_A == 0 && ls.switch_B == 1) {
			m_deviation = ls.turnout_B;
			m_dir_is_ab = false;
			return true;
		}

		return false;
	}

	void GetImages(ImagesCollection &collection) override {
		if(m_dir_is_ab) {
			collection.a_in_image = m_deviation ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = MainOutSignal::Images::RED;
		} else if(!m_dir_is_ab) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = m_deviation? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.b_out_image = MainOutSignal::Images::RED;
		}
		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
	}
private:
	bool m_dir_is_ab;
	bool m_deviation; ///< True of crossing train in direction AB is over deviating turnout
};

class OutState : public State {
public:
	OutState(): State(), m_out_a(true), m_out_b(true), m_dev_a(false), m_dev_b(false){}
	virtual ~OutState() {}

	bool IsValid(const LogicState &ls) override {
		if(ls.shift_A || ls.shift_B)
			return false;


		if(ls.switch_A == -1 && ls.switch_B == 0) {
			m_dev_a = ls.turnout_A;
			m_dev_b = ls.turnout_B;
			m_out_a = true;
			m_out_b = false;
			return true;
		}

		if(ls.switch_A == 0 && ls.switch_B == -1) {
			m_dev_a = ls.turnout_A;
			m_dev_b = ls.turnout_B;
			m_out_a = false;
			m_out_b = true;
			return true;
		}

		if(ls.switch_A == -1 && ls.switch_B == -1) {
					m_dev_a = ls.turnout_A;
					m_dev_b = ls.turnout_B;
					m_out_a = true;
					m_out_b = true;
					return true;
		}

		return false;
	}

	void GetImages(ImagesCollection &collection) override {
		collection.a_in_image = MainInSignal::Images::RED;
		collection.a_out_image = MainOutSignal::Images::RED;
		collection.b_in_image = MainInSignal::Images::RED;
		collection.b_out_image = MainOutSignal::Images::RED;
		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
		if(m_out_a) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = m_dev_a ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
		}
		if(m_out_b) {
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = m_dev_b ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
		}
	}
private:
	bool m_out_a;
	bool m_out_b;
	bool m_dev_a; ///< True of crossing train in direction AB is over deviating turnout
	bool m_dev_b;
};

class InToStopOutState : public State {
public:
	InToStopOutState(): State(), m_in_a(true), m_deviation(false){}
	virtual ~InToStopOutState() {}

	bool IsValid(const LogicState &ls) override {
		if(ls.shift_A || ls.shift_B)
			return false;

		if(ls.turnout_A == ls.turnout_B)
			return false;

		if(ls.switch_A == 1 && ls.switch_B == -1) {
			m_deviation = ls.turnout_A;
			m_in_a = true;
			return true;
		}

		if(ls.switch_A == -1 && ls.switch_B == 1) {
			m_deviation = ls.turnout_B;
			m_in_a = false;
			return true;
		}

		return false;
	}

	void GetImages(ImagesCollection &collection) override {
		if(m_in_a) {
			collection.a_in_image = m_deviation ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = m_deviation ? MainOutSignal::Images::GREEN_STRAIGHT : MainOutSignal::Images::GREEN_DEVIATE;
		} else {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = m_deviation ? MainOutSignal::Images::GREEN_STRAIGHT : MainOutSignal::Images::GREEN_DEVIATE;
			collection.b_in_image = m_deviation ? MainInSignal::Images::GREEN_DEVIATE : MainInSignal::Images::GREEN_STRAIGHT;
			collection.b_out_image = MainOutSignal::Images::RED;
		}

		collection.a_shift_image = TallShift::Images::OFF;
		collection.b_shift_image = TallShift::Images::OFF;
	}
private:
	bool m_in_a;
	bool m_deviation; ///< True of crossing train in direction AB is over deviating turnout
};

class ShiftingState : public State {
public:
	ShiftingState(): State(), m_shift_a(false),m_shift_b(false),
			m_dev_a(false),m_dev_b(false),
			m_out_a(false), m_out_b(false){}

	virtual ~ShiftingState() {}

	bool IsValid(const LogicState &ls) override {
		if(!ls.shift_A && !ls.shift_B)
			return false;


		m_shift_a = ls.shift_A;
		m_dev_b = ls.turnout_B;
		m_out_b = (ls.switch_B == -1);

		m_shift_b = ls.shift_B;
		m_dev_a = ls.turnout_A;
		m_out_a = (ls.switch_A == -1);

		return true;

	}

	void GetImages(ImagesCollection &collection) override {
		if(m_shift_a && !m_shift_b && m_out_b) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = m_dev_b ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
			collection.a_shift_image = TallShift::Images::ON;
			collection.b_shift_image = TallShift::Images::OFF;
		} else if(m_shift_a && !m_shift_b && !m_out_b) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = MainOutSignal::Images::RED;
			collection.a_shift_image = TallShift::Images::ON;
			collection.b_shift_image = TallShift::Images::OFF;
		} else if(m_shift_b && !m_shift_a && m_out_a) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = m_dev_a ? MainOutSignal::Images::GREEN_DEVIATE : MainOutSignal::Images::GREEN_STRAIGHT;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = MainOutSignal::Images::RED;
			collection.b_shift_image = TallShift::Images::ON;
			collection.a_shift_image = TallShift::Images::OFF;
		} else if(m_shift_b && !m_shift_a && !m_out_a) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = MainOutSignal::Images::RED;
			collection.b_shift_image = TallShift::Images::ON;
			collection.a_shift_image = TallShift::Images::OFF;
		} else if(m_shift_b && m_shift_a) {
			collection.a_in_image = MainInSignal::Images::RED;
			collection.a_out_image = MainOutSignal::Images::RED;
			collection.b_in_image = MainInSignal::Images::RED;
			collection.b_out_image = MainOutSignal::Images::RED;
			collection.b_shift_image = TallShift::Images::ON;
			collection.a_shift_image = TallShift::Images::ON;
		}
	}
private:
	bool m_shift_a;
	bool m_shift_b;
	bool m_dev_a; ///< True of crossing train in direction AB is over deviating turnout
	bool m_dev_b; ///< True of crossing train in direction AB is over deviating turnout
	bool m_out_a;
	bool m_out_b;
};

#endif /* STATES_H_ */
