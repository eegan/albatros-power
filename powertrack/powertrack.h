/////////////////////////////////////////////////////////////////////////////////////////////////
// powertrack.h
// Miscellaneous definitions common to multiple modules in powertrack application
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef POWERTRACK_H
#define POWERTRACK_H

#define CRLF "\r\n"

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))


#define DEBUG 1

#if DEBUG
#define ASSERT(x) {if (0==(x)) assertfail(__FILE__, __LINE__);}
//#define BC(array, index) { ASSERT(index >= 0 && index < COUNT_OF (array)); }
#define BC(array, index) { bc(COUNT_OF(array), index, __FILE__, __LINE__); }
#else
#define ASSERT(x)
#endif


// Hardware definitions
#define loadPin 5     // On/off control for load switch
#define greenLEDPin 6 // pretend an LED is the load switch
#define redLEDPin 7   // pretend an LED is the load switch

#define SD_CSpin 10   // Chip-select pin for SD

#define DAY_SECONDS 86400

// Arduino is broken
// These enum declarations work (into a global scope) if it is here, but not if it is in victron.ino.
// Please maintain them in both places, until we figure out what to do.

// from victron module
enum victronFieldEnum {FI_V,     FI_VPV,   FI_PPV,   FI_I,   FI_IL, 
      FI_ILOAD, FI_Relay, FI_H19,   FI_H20, FI_H21, 
      FI_H22,   FI_H23,   FI_ERR,   FI_CS,  FI_FW, 
      FI_PID, FI_SER, FI_HSDS, FI_MPPT, FI_Checksum,
      FI_field_count
};

// from logger module
enum logVarType {
  lvtNumeric      // numeric value, will accumulate averaged/min/max
 ,lvtEnumSample   // enum value, will sample current value
 ,lvtEnumAccum    // enum value, will accumulate bitmap of values seen in log entry interval
};


// Status codes
enum { statusSDAccessError, statusVictronTimeoutError, statusVictronErrorState, statusBatteryLowError, statusRTCAccessError, statusNErrors };

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
  long m = millis();
  if (m - lastRun >= interval) {
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
