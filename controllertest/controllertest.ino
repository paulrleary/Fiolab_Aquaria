#include "SingleController.h"
#include "DualController.h"

SingleController DO;
DualController T(3, 2);

void setup() {
  DO.setPin(7);
  DO.setCyclePeriod(2000);
  DO.setDutyCycle(0.25);
  DO.setDirection(-1);
  DO.updateSetRange(2, 3);

  T.setAllCyclePeriod(200);
  T.setAllDutyCycle(0.5);
  T.updateSetRange(700, 800);


}
void loop() {
    DO.updateControl(4);
    T.updateControl(analogRead(A0));
 
}
