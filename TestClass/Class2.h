#ifndef Class2_h
#define Class2_h

#include "Arduino.h"
#include "Class1.h"
class Class2 : public Class1
{
public:
int z;

Class2(int x = Class1.getX(), int y = Class1.getY(), int z = -572876);

int getZ();
	
};
#endif
