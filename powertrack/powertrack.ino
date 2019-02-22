const unsigned long loopInterval = 200;
const unsigned long mainLoopSleepTimeMs = 100;
unsigned long lastLoopBegin;

typedef unsigned long UINT32;
typedef unsigned short UINT16;

void setup()
{
  lastLoopBegin = millis();
  serviceDatastreamInit();
  debugInit();
}

void loop()
{
  // Pace the main loop
  // TODO: implement sleep
  #if 0
  long sinceLastLoopBegin;
  unsigned long now;
  do {
    delay(mainLoopSleepTimeMs); // TODO: replace with power-down sleep, but maybe not - maybe just during non-reception of serial chars
    now = millis();
    sinceLastLoopBegin = now - lastLoopBegin;
  }
  while (sinceLastLoopBegin < loopInterval);
  lastLoopBegin = now;
  #endif
  serviceDatastream();
}


////////////////////////////////////////////////////////////////////////////////////
// Assert support
////////////////////////////////////////////////////////////////////////////////////

#define DEBUG 1

#if DEBUG
#define ASSERT(x) {if (0==(x)) assertfail(__FILE__, __LINE__);}
#else
#define ASSERT(x)
#endif

void assertfail(char *file, long line)
{
  // TODO: something
  // like print a message on the debug port
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
// Debug support
////////////////////////////////////////////////////////////////////////////////////
HardwareSerial &debug = Serial;
void debugInit()
{
  debug.begin(9600);
}


/////////////////////////////////////////////////////////////////////////////////////
// Serial port service
// Allow bytes to accumulate in serial buffer until serialTimeoutMs has elapsed and no
// new bytes have been received. Then call ParsePacket.
//
// To be called periodically from main loop.
// Relies on the timing of the Victron data output in order to segregate one complete packet.
// Victron sends its data in a burst once per second. We rely on the bytes being grouped together in time.
/////////////////////////////////////////////////////////////////////////////////////

HardwareSerial &victronData = Serial1; // Serial port used to receive Victron data. On Arduino Mega 2560, this can point to Serial1, 2, or 3; on others, to Serial
unsigned long lastCharRxMs;           // millisecond timestamp when last increment of available bytes was detected
unsigned short lastCharCount;         // Tracks fullness of serial buffer
const unsigned long serialTimeoutMs = 5;  // Time after last character received, to declare the packet received and parse it

void serviceDatastreamInit()
{
//  while (!victronData) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }

  victronData.begin(19200);  // Victron baud rate
  victronData.setTimeout(0); // this apparently works to set infinite timeout which is what we want

  initbuffers();
}

const int CBLEN = 250;
const int LBBLEN = 10;
const char cbstring[] = "sum\t";
const int CBRETAIN = 5; // length of string + 1 for the actual checksum

char circbuf[CBLEN];
char lbbuf[LBBLEN];

UINT16 cbin, cbout, lbin;

void initbuffers()
{
  debug.println("initializing");
  cbout = cbin = lbin = 0;
}

// Returns true on the call when the packet gets parsed
bool serviceDatastream()
{
    // while we have data to read
#if 1
    while(victronData.available()) {
      char c = victronData.read();
#else
    while(debug.available()) {
      char c = debug.read();
#endif
      // put it in the circular buffer
      circbuf[cbin++] = c;
      if (cbin == CBLEN) cbin = 0;  // wrap input pointer
      if (cbout == cbin) {
        cbout++;   // if buffer overflowed, drop the oldest character by bumping output pointer
        if (cbout == CBLEN) cbout = 0;  // wrap output pointer
      }

      // check whether lookback buffer is full, if so, make room, copying CBRETAIN bytes back to start
      if (lbin == LBBLEN) {
        memmove(lbbuf, lbbuf+LBBLEN-CBRETAIN, CBRETAIN);
        lbin = CBRETAIN;  // adjust byte count
      }

      // add character to lookback buffer
      lbbuf[lbin++] = c;

      // check for match and parse if so
      if (0 == memcmp(cbstring, lbbuf+lbin-CBRETAIN, CBRETAIN-1)) {
        debug.print("cbin :"); debug.println(cbin);
        debug.print("cbout:"); debug.println(cbout);        
        ParsePacket();
        initbuffers();
      }
    }    
}

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// Buffer for reading elements (field tag and field value)
const int buflen = 20;
char buf[buflen];

////////////////////////
// Victron field table
////////////////////////
struct fd {
  char *tag;  // the string we expect to see from the Victron
  byte index; // an index into the result array
  byte type;  // Type enumeration  so we know what to expect and how to treat it 
};

// Field type enumerations
const byte FT_int =     1;  // millivolts
const byte FT_ON_OFF =  2;  // "OFF" / "ON" in string, --> 0 / 1
const byte FT_bool =    3;  // 0 or 1
const byte FT_string =  4;
const byte FT_checksum = 5; // single byte, could be anything, special treatment

const byte FI_V     = 0;
const byte FI_VPV   = 1;
const byte FI_PPV   = 2;
const byte FI_I     = 3;
const byte FI_IL    = 4;
const byte FI_ILOAD = 5;
const byte FI_Relay = 6;
const byte FI_H19   = 7;
const byte FI_H20   = 8;
const byte FI_H21   = 9;
const byte FI_H22   = 10;
const byte FI_H23   = 11;
const byte FI_ERR   = 12;
const byte FI_CS    = 13;
const byte FI_FW    = 14;
const byte FI_PID   = 15;
const byte FI_SER   = 16;
const byte FI_HSDS  = 17;
const byte FI_MPPT  = 18;
const byte FI_Checksum = 19;

struct fd fieldDescriptors[] = {
   {"V",      FI_V,     FT_int} 
  ,{"VPV",    FI_VPV,   FT_int} 
  ,{"PPV",    FI_PPV,   FT_int} 
  ,{"I",      FI_I,     FT_int} 
  ,{"IL",     FI_IL,    FT_int} 
  ,{"LOAD",   FI_ILOAD, FT_ON_OFF} 
  ,{"Relay",  FI_Relay, FT_ON_OFF} 
  ,{"H19",    FI_H19,   FT_int} 
  ,{"H20",    FI_H20,   FT_int} 
  ,{"H21",    FI_H20,   FT_int} 
  ,{"H22",    FI_H20,   FT_int} 
  ,{"H23",    FI_H20,   FT_int} 
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

void ParsePacket()
{

  byte fieldIndex;
  byte fieldType;


  char c;
//  debug.write("\n>>>");
  while (circbuf[cbout] != 'P')
    nextchar();

  // For now, just spit out the packet
//  while (available())
//    debug.write(nextchar());
//  debug.write("<<<\n");

  // initialize checksum to what came right before the 'P' we scanned for
  char checksum = '\r' + '\n';
  
  // For each field
  while (available()) {
    
    //debug.print("available: "); debug.println(available());    
    
    // read the field label into buffer
    readElement(checksum, '\t');
    debug.print("Element: "); debug.println(buf);
    
    // search for field tag
    fieldIndex = -1;
    for (int i=0; i<COUNT_OF(fieldDescriptors); i++) {
      if (0 == strcasecmp(buf, fieldDescriptors[i].tag)) {
        fieldIndex = fieldDescriptors[i].index;
        fieldType = fieldDescriptors[i].type;
      }
    }

    // TODO: probably don't make this an assertion, since we want to work normally
    // if a controller has additional fields we don't know about
    ASSERT(-1 != fieldIndex) // or it wasn't found :(

    // read the field value into buffer
    readElement(checksum, '\r');

    // Checksum and discard the linefeed between fields (if it's there). It should be 
    // there, unless we're at the last (checksum) field
    if (available()) {
      char c = nextchar();
      ASSERT('\n' == c)
      checksum += c; // should be '\n'
    }

    long value;
    
    switch(fieldType) {
      case FT_int:
      case FT_bool:
        value = atol(buf);
        
        debug.print(fieldDescriptors[fieldIndex].tag);
        debug.print("=");
        debug.println(value);
        
        // TODO: something with value
        break;
      case FT_ON_OFF:
        value = 0 == strcasecmp("on",buf);  // aka stricmp
        break;
      case FT_checksum:
        ASSERT(0 == checksum);
        // do something with this information
        break;
      default:
        ASSERT(0);
    }
  }

 // TODO: check if checksum = 0, or just store it    
   
}

char nextchar() {
  char c = circbuf[cbout++];  
  if (cbout >= CBLEN) cbout = 0;
  return c;
}

int available() {
  int count = cbin - cbout;
  if (count < 0) count += CBLEN;
  return count;
}

//void readElement();

void readElement(char &checksum, char terminator) {

  // Prevent buffer overrun (use buflen, leaving room for terminal null)
  // Only expect up to amount that was available in packet (nchars): The last field won't 
  // terminate in \r\n, since this is at the start of the next packet
  int maxToRead = min(buflen, available());
  int ellen;  // element length
  for (ellen = 0; ellen < maxToRead; ellen++) {
    buf[ellen] = nextchar();
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
  buf[ellen] = '\0';  
}
