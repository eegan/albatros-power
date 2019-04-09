#ifndef POWERTRACK_H
#define POWERTRACK_H
typedef unsigned long UINT32;
typedef unsigned short UINT16;
typedef long INT32;
typedef short INT16;

typedef long logDataType;

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define loadPin 5 // On/off control for load switch
#define greenLEDPin 6 // pretend an LED is the load switch
#define redLEDPin 7 // pretend an LED is the load switch

#define DAY_SECONDS 86400

#define statusSDError 1

#endif
