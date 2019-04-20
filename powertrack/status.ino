/////////////////////////////////////////////////////////////////////////////////////
// Diagnostic status
/////////////////////////////////////////////////////////////////////////////////////

runTimer statusRunTimer(50);  // run interval

/////////////////////////////////////////////////////////////////////////////////////
// statusInit
// Init (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void statusInit()
{
    pinMode(greenLEDPin, OUTPUT);
    pinMode(redLEDPin, OUTPUT);
}


/////////////////////////////////////////////////////////////////////////////////////
// Status tracking code
/////////////////////////////////////////////////////////////////////////////////////
struct {
  const char *name;
  bool value;
  bool blocked;
  bool latching;
}
status[] = 
{
   {"SD Card Access error", false, false, true}
  ,{"Victron data timeout", false, false, true}
  ,{"Victron error state" , false, false, true}
  ,{"Battery voltage low" , false, false, true}
  ,{"RTC access error"    , false, false, true}
  
}
;



void statusReportStatus(uint16_t code, bool conditionTrue)
{
  if (code >= COUNT_OF(status)) return;
  if ( conditionTrue ) {
    // reporting true
    if ( ! status[code].blocked )
      status[code].value = true;
  }
  else {
    // reporting false
    if ( ! status[code].latching )
      status[code].value = false;
  }
}


void statusSetLatching(uint16_t code, bool state)
{
  if (code >= COUNT_OF(status)) return;
  status[code].latching = state;
}

void statusSetBlocked(uint16_t code, bool state)
{
  if (code >= COUNT_OF(status)) return;  
  status[code].blocked = state;
  if (state) status[code].value = false;
}

void statusClear(uint16_t code)
{
  if (code >= COUNT_OF(status)) return;
  status[code].value = false;
}

void statusClearAll()
{
  for (uint16_t i=0; i<COUNT_OF(status); i++)
    status[i].value = false;
}

void statusDumpStatus(Stream &p) {

  for (uint16_t i=0; i<COUNT_OF(status); i++)
  {
    char buf[50];
    sprintf(buf, "%d - %s  =  %s, %s, %s", 
        i 
      , status[i].name 
      , status[i].value    ? "ACTIVE"  : "inactv"
      , status[i].latching ? "LATCHED" : "nolatch"
      , status[i].blocked  ? "BLOCKED" : "noblock"
      );
    p.println(buf);
  }
}


/////////////////////////////////////////////////////////////////////////////////////
// LED interface code
/////////////////////////////////////////////////////////////////////////////////////
bool statusLoadOn = false;            // local copy of load on, as indicated by notification
bool errorStatus = false;             // indicates error alert state
int blinkState = 0;
const int maxBlinkState = 20;


void statusNotifyLoad(bool on)
{
  statusLoadOn = on;
}

void statusLoopHandler()
{
  if (statusRunTimer.runNow()) {  
    bool errorState = false;
    for (uint16_t i = 0; i< COUNT_OF(status); i++)
      errorState |= status[i].value;    
    
    blinkState++;
    if (blinkState > maxBlinkState) blinkState = 0;
    bool LEDBlink = 0 == blinkState;

    bool greenOn, redOn;
    if (!LEDBlink)  // most of the time
    {
      greenOn = statusLoadOn && !errorStatus;
      redOn   = statusLoadOn &&  errorStatus;
    }
    else            // during a short blink
    {
      greenOn = statusLoadOn || !errorStatus;
      redOn   = statusLoadOn ||  errorStatus;
    }
    
    digitalWrite(greenLEDPin, greenOn);
    digitalWrite(redLEDPin,   redOn);
  }

}
