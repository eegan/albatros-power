/////////////////////////////////////////////////////////////////////////////////////////////////
// powertrack.h
// Miscellaneous definitions common to multiple modules in powertrack application
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef POWERTRACK_H
#define POWERTRACK_H

typedef unsigned long UINT32;
typedef unsigned short UINT16;
typedef long INT32;
typedef short INT16;

#define CRLF "\r\n"

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// Hardware definitions
#define loadPin 5     // On/off control for load switch
#define greenLEDPin 6 // pretend an LED is the load switch
#define redLEDPin 7   // pretend an LED is the load switch

#define SD_CSpin 10   // Chip-select pin for SD

#define DAY_SECONDS 86400

// Status codes
enum { statusSDError, statusVictronTimeoutError, statusBatteryLowError };


class runTimer
{
  long lastRun;
  long interval;
public:
  runTimer(long);
  bool runNow();
  void reinit();
};

runTimer::runTimer(long intrval): interval(intrval)
{
  reinit();
}

void runTimer::reinit()
{
  lastRun = millis();  
}

bool runTimer::runNow()
{
  long m;
  if ((m = millis()) - lastRun >= interval) {
    lastRun = m;
    return true;
  }
  return false;
}

#if 0
/////////////////////////////////////////////////////////////////////////////////////////////////
// Double hardwareserial function
/////////////////////////////////////////////////////////////////////////////////////////////////

class DoubleHWS
: public Stream
{
  HardwareSerial &myh1;
  HardwareSerial &myh2;
 
 public:   
  DoubleHWS(HardwareSerial, HardwareSerial);
  void begin(unsigned long, uint8_t);
  void begin(unsigned long);
  virtual int available(void);
  virtual int peek(void);
  virtual int read(void);
  virtual size_t write(uint8_t);
};

DoubleHWS::DoubleHWS (HardwareSerial h1, HardwareSerial h2)
: myh1(h1), myh2(h2)
{
  
}

void DoubleHWS::begin(unsigned long baud, byte config)
{
  myh1.begin(baud, config);
  myh2.begin(baud, config);
//  Serial.println("dhws begin");
}

void DoubleHWS::begin(unsigned long baud)
{
  begin(baud, SERIAL_8N1);
//  Serial.begin(9600);
//  Serial.println("dhws begin");
}

int DoubleHWS::available(void)
{
  return myh1.available() || myh2.available();
}

int DoubleHWS::read(void)
{
  int retval = myh1.read();
  if (EOF == retval) retval = myh2.read();
}

int DoubleHWS::peek(void)
{
  int retval = myh1.peek();
  if (EOF == retval) retval = myh2.peek();
}

size_t DoubleHWS::write(uint8_t byte)
{
  myh1.write(byte);
  myh2.write(byte);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
#endif 


#endif
