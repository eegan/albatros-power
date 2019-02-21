const unsigned long loopInterval = 200;
const unsigned long mainLoopSleepTimeMs = 100;
unsigned long lastLoopBegin;

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
    
  lastCharCount = 0;
  lastCharRxMs = millis();
}

// Returns true on the call when the packet gets parsed
bool serviceDatastream()
{
  unsigned long now = millis();

  // If there are no bytes in the buffer, return false
  if (0 == victronData.available())
    return false;

  // If there are bytes in the buffer, and if there is a new one since the last run,
  // reset the indicators (character count and last time a character was seen)
  if (victronData.available() > lastCharCount) {
    lastCharCount = victronData.available();
    lastCharRxMs = now;
    return false;
  }

  // If no new characters arrived in the buffer (if they did, code would have returned above), and if elapsed time since last 
  // character was received is beyond the timeout threshold, parse the packet, reset indicators to empty, and return true.
  if (now - lastCharRxMs > serialTimeoutMs) {

//    debug.println("VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");
//    int i = lastCharCount;
//    while(i--)
//      debug.write(victronData.read());
//    debug.println("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

    ParsePacket(lastCharCount);
    
    lastCharCount = 0;
    lastCharRxMs = now;

    // TODO: handle case where packet parse fails
    return true;
  }

  // else, there are no new bytes in the buffer, but we haven't reached the timeout yet
  else
    return false;
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

void ParsePacket(long nchars)
{
  #if true
  byte fieldIndex;
  byte fieldType;
  
  // read through the first newline
  nchars -= victronData.readBytes(buf, 2);

  // initialize value of checksum to what we expect to read first
  char checksum = '\r' + '\n';
  
  ASSERT (buf[0] == '\r' && buf[1] == '\n')

  // For each field
  while (nchars) {
    
    debug.print("Nchars: "); debug.println(nchars);    
    
    // read the field label into buffer
    readElement(nchars, checksum, '\t');
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
    readElement(nchars, checksum, '\r');

    // Checksum and discard the linefeed between fields (if it's there). It should be 
    // there, unless we're at the last (checksum) field
    if (nchars > 0) {
      nchars --;
      char c = victronData.read();
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
   
  #else
  
  // For now, just spit out the packet
  int c;
  victronData.write("\n>>>");
  while ((c = victronData.read()) != -1)
    victronData.write(c);
  victronData.write("<<<\n");
  #endif
}

//void readElement();

void readElement(long &nchars, char &checksum, char terminator) {

  // Prevent buffer overrun (use buflen, leaving room for terminal null)
  // Only expect up to amount that was available in packet (nchars): The last field won't 
  // terminate in \r\n, since this is at the start of the next packet
  int maxToRead = min(buflen, nchars);
  int ellen;  // element length
  for (ellen = 0; ellen < maxToRead; ellen++) {
    buf[ellen] = victronData.read();
    checksum += buf[ellen];
    if (buf[ellen] == terminator)
      break;
  }

  // subtract ellen+1 for the terminator (if seen) or ellen if not seen
  nchars -= buf[ellen] == terminator? (ellen +1) : ellen;

  // In most cases, the terminator is the last character read
  // Only for the last element (checksum value) do we not expect to necessarily see the terminator, 
  // since actually the \r\n is prepended to the start of each field, and will show up at the start
  // of the next packet. In this instance, nchars should be zero since we've read the whole thing.
   
  ASSERT(buf[ellen] == terminator || nchars == 0)
  // replace the terminator so it's a nice string
  // note that we will also overwrite the checksum byte but that's okay since we've checksummed it
  // and we'll check the checksum later
  buf[ellen] = '\0';  
}
