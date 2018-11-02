#include "DualController.h"

DualController::DualController(int negativecontrollerpin, int positivecontrollerpin,  float negativecontrollercycleperiod = 0, float positivecontrollercycleperiod = 0, float negativecontrollerdutycycle = 0, float positivecontrollerdutycycle = 0){
		NegativeController.setPin(negativecontrollerpin);
		NegativeController.setCyclePeriod(negativecontrollercycleperiod);
		NegativeController.setDutyCycle(negativecontrollerdutycycle);
		NegativeController.setDirection(-1);

		PositiveController.setPin(positivecontrollerpin);
		PositiveController.setCyclePeriod(positivecontrollercycleperiod);
		PositiveController.setDutyCycle(positivecontrollerdutycycle);
		PositiveController.setDirection(1);
	}

DualController::~DualController(){}

void DualController::setAllCyclePeriod(float cycleperiod){
	NegativeController.setCyclePeriod(cycleperiod);
	PositiveController.setCyclePeriod(cycleperiod);
}
void DualController::setAllDutyCycle(float dutycycle){
	NegativeController.setDutyCycle(dutycycle);
	PositiveController.setDutyCycle(dutycycle);
}
void DualController::setAllRangeBuffer(float rangebuffer){
	NegativeController.setRangeBuffer(rangebuffer);
	PositiveController.setRangeBuffer(rangebuffer);
}

void DualController::setNegativePin(int pin){
	NegativeController.setPin(pin);
}
void DualController::setNegativeCyclePeriod(float cycleperiod){
	NegativeController.setCyclePeriod(cycleperiod);
}
void DualController::setNegativeDutyCycle(float dutycycle){
	NegativeController.setDutyCycle(dutycycle);
}
void DualController::setNegativeRangeBuffer(float rangebuffer){
	NegativeController.setRangeBuffer(rangebuffer);
}

void DualController::setPositivePin(int pin){
	PositiveController.setPin(pin);
}
void DualController::setPositiveCyclePeriod(float cycleperiod){
	PositiveController.setCyclePeriod(cycleperiod);
}
void DualController::setPositiveDutyCycle(float dutycycle){
	PositiveController.setDutyCycle(dutycycle);
}
void DualController::setPositiveRangeBuffer(float rangebuffer){
	PositiveController.setRangeBuffer(rangebuffer);
}

void DualController::updateSetRange(float min, float max){
	NegativeController.updateSetRange(min, max);
	PositiveController.updateSetRange(min, max);
}

void DualController::updateControl(float currentreading){
	NegativeController.updateControl(currentreading);
	PositiveController.updateControl(currentreading);
}
