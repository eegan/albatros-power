/////////////////////////////////////////////////////////////////////////////////////
// Victron VE.DIRECT serial data stream parsing
/////////////////////////////////////////////////////////////////////////////////////
#include "powertrack.h"

#define DEBUG_VICTRON 0
HardwareSerial &victronData = Serial2; // Serial port used to receive Victron data. On Arduino Mega 2560, this can point to Serial1, 2, or 3; on others, to Serial
unsigned long lastCharRxMs;           // millisecond timestamp when last increment of cb_available bytes was detected
unsigned short lastCharCount;         // Tracks fullness of serial buffer
const unsigned long serialTimeoutMs = 5;  // Time after last character received, to declare the packet received and parse it

int32_t victronLastSampleTime;
bool victronSampleReceived = false;

/////////////////////////////////////////////////////////////////////////////////////
// victronInit
// Init (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void victronInit()
{
  victronData.begin(19200);  // Victron baud rate
  victronData.setTimeout(0); // this apparently works to set infinite timeout which is what we want
  
  victronLastSampleTime = millis();
  
  initbuffers();
  initVictronLogging();
}

struct {
  char const *name;       // name for CSV header
  uint16_t sourceIndex;  // index into parse array
  logVarType type;  // type
  uint16_t logIndex;

} victronLogVariables[] =

{
   // Assuming this (or some Victron element) is element zero,
   // for the purpose of printing a representative sampleCount in the log
   {"BatVolt",  FI_V,   lvtNumeric}
  ,{"PVVolt",   FI_VPV, lvtNumeric}
  ,{"PVPwr",    FI_PPV, lvtNumeric}
  ,{"BatCur",   FI_I,   lvtNumeric}
  ,{"Error",    FI_ERR, lvtEnumSample}
  ,{"State",    FI_CS,  lvtEnumAccum}
  ,{"MPPT",     FI_MPPT,  lvtEnumAccum}
//  ,{"Load",     -1,  lvtEnumAccum}      // hack (special case)
};

void initVictronLogging()
{
  for (uint16_t i = 0; i<COUNT_OF(victronLogVariables); i++)
  {
    const char *name = victronLogVariables[i].name;
    logVarType type = victronLogVariables[i].type;
    uint16_t sourceIndex = victronLogVariables[i].sourceIndex;
    victronLogVariables[i].logIndex = loggerRegisterLogVariable(name, type);
  }
}

// TODO: port this to Victron, except have our own array that includes Victron index as well as logger index
// receive notification of a new Victron sample
void logVictronSample()
{
  for (uint16_t i=0; i < COUNT_OF(victronLogVariables); i++) {
    int logIndex = victronLogVariables[i].logIndex;
    int sourceIndex = victronLogVariables[i].sourceIndex;

    // get the Victron data field, unless it's the load we are logging
    long value = victronGetFieldValue(sourceIndex);
    // Note: strictly speaking it doesn't make sense to tie the load state sampling to Victron data packets
    // Logically this should be done independently. But practically speaking, if the Victron stops sending
    // data, it probably means something pretty bad.
    loggerLogSample(logIndex, value);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Circular buffer
///////////////////////////////////////////////////////////////////////////////////////////
const int CBLEN = 250;
const char cbstring[] = "sum\t";
const int CBRETAIN = 5; // length of string + 1 for the actual checksum

char circbuf[CBLEN];

uint16_t cbin, cbout;
void cb_insert(char c)
{
      // put it in the circular buffer
      BC(circbuf, cbin);
      circbuf[cbin++] = c;
      if (cbin >= CBLEN) cbin = 0;  // wrap input pointer
      if (cbout == cbin) {
        cbout++;   // if buffer overflowed, drop the oldest character by bumping output pointer
        if (cbout >= CBLEN) cbout = 0;  // wrap output pointer
      }
}

char cb_nextchar() {
  char c = circbuf[cbout++];  
  if (cbout >= CBLEN) cbout = 0;
  return c;
}

int cb_available() {
  int count = cbin - cbout;
  if (count < 0) count += CBLEN;
  return count;
}

void cbinit() {
  cbin = cbout = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Look-back buffer
///////////////////////////////////////////////////////////////////////////////////////////
const int LBBLEN = 10;
char lbbuf[LBBLEN];
uint16_t lbin;
void lbbinit() {
  lbin = 0;
}

// initialize state
void initbuffers()
{
  //debug.println("initializing");
  cbinit();
  lbbinit();
}

/////////////////////////////////////////////////////////////////////////////////////
// victronLoopHandler
// Loop handler (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void victronLoopHandler()
{
    // while we have data to read
    while(0 != victronData.available()) {
      char c = victronData.read();
      cb_insert(c);

      // check whether lookback buffer is full, if so, make room, copying CBRETAIN bytes back to start
      if (lbin == LBBLEN) {
        memmove(lbbuf, lbbuf+LBBLEN-CBRETAIN, CBRETAIN);
        lbin = CBRETAIN;  // adjust byte count
      }

      // add character to lookback buffer
      BC(lbbuf, lbin);
      lbbuf[lbin++] = c;

      // check for match and parse if so
      if (0 == memcmp(cbstring, lbbuf+lbin-CBRETAIN, CBRETAIN-1)) {
        ParsePacket();
        initbuffers();
      }
    }    
}


///////////////////////////////////////////////////////////////////////////////////////////
// Parser
///////////////////////////////////////////////////////////////////////////////////////////

// Field table
struct fd {
  char const *tag;  // the string we expect to see from the Victron
  byte index; // an index into the result array
  byte type;  // Type enumeration  so we know what to expect and how to treat it 
  long value; // we will store the value in the array, once it's parsed, except for strings
};

// Field type enumerations
enum {FT_int, FT_ON_OFF, FT_bool, FT_string, FT_checksum};

// Field index enumerations
//enum fubar {FI_V};


// Arduino is broken
// This enum declaration works (into a global scope) if it is elsewhere but not if it is here
// It is in powertrack.h
// Please maintain them in both places, until we figure out what to do.
//enum victronFieldEnum {FI_V,     FI_VPV,   FI_PPV,   FI_I,   FI_IL, 
//      FI_ILOAD, FI_Relay, FI_H19,   FI_H20, FI_H21, 
//      FI_H22,   FI_H23,   FI_ERR,   FI_CS,  FI_FW, 
//      FI_PID, FI_SER, FI_HSDS, FI_MPPT, FI_Checksum,
//      FI_field_count
//};


struct fd fieldDescriptors[] = {
   {"V",      FI_V,     FT_int}   // main battery voltage (mV)
  ,{"VPV",    FI_VPV,   FT_int}   // panel voltage (mV)
  ,{"PPV",    FI_PPV,   FT_int}   // panel power (W)
  ,{"I",      FI_I,     FT_int}   // battery current (mA, signed)
  ,{"IL",     FI_IL,    FT_int}   // load current (does not appear on 150/35)
  ,{"LOAD",   FI_ILOAD, FT_ON_OFF} // load state (does not appear on 150/35)
  ,{"Relay",  FI_Relay, FT_ON_OFF}  // relay state (does not appear on 150/35)
  ,{"H19",    FI_H19,   FT_int}   // total yield (resettable) x 0.01kWh
  ,{"H20",    FI_H20,   FT_int}   // yield today x 0.01kWh
  ,{"H21",    FI_H21,   FT_int}   // max power today (W)
  ,{"H22",    FI_H22,   FT_int}   // yield yesterday x 0.01kWh
  ,{"H23",    FI_H23,   FT_int}   // max power yesterday (W)
  ,{"ERR",    FI_ERR,   FT_int} 
  ,{"CS",     FI_CS,    FT_int} 
  ,{"FW",     FI_FW,    FT_string} 
  ,{"PID",    FI_PID,   FT_string} 
  ,{"SER#",   FI_SER,   FT_string} 
  ,{"HSDS",   FI_HSDS,  FT_int} 
  ,{"MPPT",   FI_MPPT,  FT_int}
  ,{"Checksum", FI_Checksum, FT_checksum} // field ID may never be used??
};

// Read and parse the Victron packet
// Refer to Victron VE.Direct Protocol document

// TODO: 
// Deal with any asserts that need coding to recover from e.g. a serial data error

// Buffer for reading elements (field tag and field value)
const int BUFLEN = 20;
char buf[BUFLEN];

void ParsePacket()
{

  int16_t fieldIndex;
  byte fieldType;

  // read until we see 'P' for PID field; there shouldn't be any in any hex data in between packets
  while (circbuf[cbout] != 'P')
    cb_nextchar();

  // initialize checksum to what came right before the 'P' we scanned for
  char checksum = '\r' + '\n';
  
  // For each field
  while (cb_available()) {
    
    //debug.print("cb_available: "); debug.println(cb_available());    
    
    // read the field label into buffer
    readElement(checksum, '\t');
    //debug.print("Element: "); debug.println(buf);
    
    // search for field tag
    fieldIndex = -1;
    for (uint16_t i=0; i<COUNT_OF(fieldDescriptors); i++) {
      if (0 == strcasecmp(buf, fieldDescriptors[i].tag)) {
        fieldIndex = fieldDescriptors[i].index;
        fieldType = fieldDescriptors[i].type;
      }
    }

    // TODO: probably don't make this an assertion, since we want to work normally
    // if a controller has additional fields we don't know about
    //ASSERT(-1 != fieldIndex) // or it wasn't found :(
    // read the field value into buffer
    readElement(checksum, '\r');

    // Checksum and discard the linefeed between fields (if it's there). It should be 
    // there, unless we're at the last (checksum) field
    if (cb_available()) {
      char c = cb_nextchar();
      //ASSERT('\n' == c)
      checksum += c; // should be '\n'
    }

    if (-1 == fieldIndex) { // unknown field 
      // monitorPort.print("Unknown field: ");
      // monitorPort.println(buf);
    }
    else {
      long value = 0;
      switch(fieldType) {
        case FT_int:
        case FT_bool:
          value = atol(buf);
          
          #if DEBUG_VICTRON
          debug.print(fieldDescriptors[fieldIndex].tag);
          debug.print("=");
          debug.println(value);
          #endif
          
          // TODO: something with value
          break;
        case FT_ON_OFF:
          value = 0 == strcasecmp("on", buf);  // aka stricmp
          break;
        case FT_checksum:
          #if DEBUG_VICTRON
          debug.print("Checksum: "); debug.println((int)checksum);
          #endif
          if (0 == checksum) {
            victronSampleReceived = true; // flag that we've seen a packet (ever)
            victronLastSampleTime = millis();
            victronUpdateNotify();
          }
          break;
        case FT_string:
          #if DEBUG_VICTRON
          debug.print(fieldDescriptors[fieldIndex].tag);
          debug.print("=");
          debug.println(value);
          #endif
          
          break;
        default:
          ASSERT(0);
      }
      BC(fieldDescriptors, fieldIndex);
      fieldDescriptors[fieldIndex].value = value;
    }
  }
}

void readElement(char &checksum, char terminator) {
  // Prevent buffer overrun (use BUFLEN, leaving room for terminal null)
  // Only expect up to amount that was cb_available in packet (nchars): The last field won't 
  // terminate in \r\n, since this is at the start of the next packet
  int maxToRead = min(BUFLEN, cb_available());
  int ellen;  // element length
  for (ellen = 0; ellen < maxToRead; ellen++) {
    BC(buf, ellen);
    buf[ellen] = cb_nextchar();
    checksum += buf[ellen];
    if (buf[ellen] == terminator)
      break;
  }

  // In most cases, the terminator is the last character read
  // Only for the last element (checksum value) do we not expect to necessarily see the terminator, 
  // since actually the \r\n is prepended to the start of each field, and will show up at the start
  // of the next packet. In this instance, nchars should be zero since we've read the whole thing.
   
  //ASSERT(buf[ellen] == terminator || nchars == 0)
  // replace the terminator so it's a nice string
  // note that we will also overwrite the checksum byte but that's okay since we've checksummed it
  // and we'll check the checksum later

  if (ellen == BUFLEN)  // In an effort to determine if we have caught a  bug, ... the lower if condition was <=
	  statusLogPrint("WOULD HAVE WRITTEN 1 LOCATION OUT OF BOUNDS");
  if (ellen < BUFLEN) {
    BC(buf, ellen);
	  buf[ellen] = '\0';
  }
}

// TODO: add string fields (they have to also be stored ...)
void victronDumpStatus(Stream &p)
{
  for (uint16_t i=0; i<FI_field_count; i++) {
    if (fieldDescriptors[i].type != FT_string) 
    {
      p.print(fieldDescriptors[i].tag);
      p.print(" = ");
      p.println(fieldDescriptors[i].value);
    }
  }
}
void victronUpdateNotify()
{
  //TODO: notify other modules
  logVictronSample();
  loadctlNotifyVictronSample();
  statusNotifyVictronSample();
}

long victronGetDataAge()
{
  return millis() - victronLastSampleTime;
}

bool victronSampleSeen()
{
  return victronSampleReceived;
}

// parameter should be type victronFieldEnum, but arduino won't let me
long victronGetFieldValue(int field)
{
  return fieldDescriptors[field].value;
}
