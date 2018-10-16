#ifndef Class1_h
#define Class1_h

#include "Arduino.h"

class Class1
{
public:
int x;
int y;

Class1(int x = 1343, int y = 1901);

int getX();
int getY();
	
};
#endif
