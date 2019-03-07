/////////////////////////////////////////////////////////////////////////////////////
// Victron VE.DIRECT serial data stream parsing
/////////////////////////////////////////////////////////////////////////////////////

HardwareSerial &victronData = Serial1; // Serial port used to receive Victron data. On Arduino Mega 2560, this can point to Serial1, 2, or 3; on others, to Serial
unsigned long lastCharRxMs;           // millisecond timestamp when last increment of available bytes was detected
unsigned short lastCharCount;         // Tracks fullness of serial buffer
const unsigned long serialTimeoutMs = 5;  // Time after last character received, to declare the packet received and parse it

void victron_serviceDatastreamInit()
{
  victronData.begin(19200);  // Victron baud rate
  victronData.setTimeout(0); // this apparently works to set infinite timeout which is what we want

  victron_initbuffers();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Circular buffer
///////////////////////////////////////////////////////////////////////////////////////////
const int CBLEN = 250;
const char cbstring[] = "sum\t";
const int CBRETAIN = 5; // length of string + 1 for the actual checksum

char circbuf[CBLEN];

UINT16 cbin, cbout;

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

void cbinit() {
  cbin = cbout = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Look-back buffer
///////////////////////////////////////////////////////////////////////////////////////////
const int LBBLEN = 10;
char lbbuf[LBBLEN];
UINT16 lbin;
void lbbinit() {
  lbin = 0;
}

// initialize state
void victron_initbuffers()
{
  //debug.println("initializing");
  cbinit();
  lbbinit();
}

// Returns true on the call when the packet gets parsed
bool victron_serviceDatastream()
{
    // while we have data to read
    while(0 != victronData.available()) {
      char c = victronData.read();
      
      // put it in the circular buffer
      circbuf[cbin++] = c;
      if (cbin >= CBLEN) cbin = 0;  // wrap input pointer
      if (cbout == cbin) {
        cbout++;   // if buffer overflowed, drop the oldest character by bumping output pointer
        if (cbout >= CBLEN) cbout = 0;  // wrap output pointer
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
        victron_parsePacket();
        victron_initbuffers();
      }
    }    
}

// Buffer for reading elements (field tag and field value)
const int BUFLEN = 20;
char buf[BUFLEN];

///////////////////////////////////////////////////////////////////////////////////////////
// Parser
///////////////////////////////////////////////////////////////////////////////////////////
logDataType parsedData[COUNT_OF(fieldDescriptors)];

// Read and parse the Victron packet
// Refer to Victron VE.Direct Protocol document

// TODO: 
// Deal with any asserts that need coding to recover from e.g. a serial data error

<<<<<<< HEAD
extern logDataType parsed[];

void victron_parsePacket() // ALWAYS modifies the same array
=======
void victron_parsePacket() //TODO: get to work with UINT16
>>>>>>> 3e66e2cb8e7bd706a434cf0df8c5144d58725b71
{

  byte fieldIndex;
  byte fieldType;

  char c;

  // read until we see 'P' for PID field; there shouldn't be any in any hex data in between packets
  while (circbuf[cbout] != 'P')
    nextchar();

  // initialize checksum to what came right before the 'P' we scanned for
  char checksum = '\r' + '\n';
  
  // For each field
  while (available()) {
    
    //debug.print("available: "); debug.println(available());    
    
    // read the field label into buffer
    victron_readElement(checksum, '\t');
    //debug.print("Element: "); debug.println(buf);
    
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
    victron_readElement(checksum, '\r');

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
        ///*
        debug.print(fieldDescriptors[fieldIndex].tag);
        debug.print("=");
        debug.println(value);
        //*/
        break;
      case FT_ON_OFF:
        value = 0 == strcasecmp("on",buf);  // aka stricmp
        break;
      case FT_checksum:
        switch(checksum%256) {
        case 0:
          value = 0;
          break;
        default:
          value = 1;
        }
        ///*
        debug.print("Checksum: "); 
        debug.println((int)checksum);
        //*/
        break;
      case FT_string:
        // TODO: deal with these. atol seems like the most sane.
        // what we should do is only at the beginning, print out in our event log, stuff like serial numberm, Victron firmware version etc
        debug.print(fieldDescriptors[fieldIndex].tag);
        debug.print("=");
        debug.println(buf);
        //*/
        value = atol(buf);
        break;
      default:
        ASSERT(0);
    }
    parsedData[fieldIndex] = value;
  }
<<<<<<< HEAD
=======

>>>>>>> 3e66e2cb8e7bd706a434cf0df8c5144d58725b71
}

void victron_readElement(char &checksum, char terminator) {

  // Prevent buffer overrun (use BUFLEN, leaving room for terminal null)
  // Only expect up to amount that was available in packet (nchars): The last field won't 
  // terminate in \r\n, since this is at the start of the next packet
  int maxToRead = min(BUFLEN, available());
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

  if (ellen <= BUFLEN) buf[ellen] = '\0';  
}
