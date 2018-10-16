#include "Class1.h"
#include "Class2.h"
Class2 c1;
//Class1 c1(1,3);
//Class2 c1;
void setup() {
  Serial.begin(9600);

  c1.x = 3;
//  c1.y = 17;
//  c1.z = 55;
  //  c1.m = 59;
}

void loop() {
int x = c1.x;
//  int x = c1.getX();
  Serial.println(x);
  delay(1000);

}
