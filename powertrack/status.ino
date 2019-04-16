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

bool statusLoadOn = false;            // local copy of load on, as indicated by notification
//bool LEDBlinkOn = false;            // indicates the ON phase if blinking
bool errorStatus = false;             // indicates error alert state
int blinkState = 0;
const int maxBlinkState = 20;

void statusNotifyLoad(bool on)
{
  statusLoadOn = on;
}

void statusLoopHandler()
{
  bool green, red;
  
  if (statusRunTimer.runNow()) {
    blinkState++;
    if (blinkState > maxBlinkState) blinkState = 0;
    bool LEDBlink = 0 == blinkState;

    if (!LEDBlink)  // most of the time
    {
      green = statusLoadOn && !errorStatus;
      red   = statusLoadOn &&  errorStatus;
    }
    else
    {
      green = statusLoadOn || !errorStatus;
      red   = statusLoadOn ||  errorStatus;
    }
    
    digitalWrite(greenLEDPin, green);
    digitalWrite(redLEDPin,   red);
  }

}

void reportStatus(int code, bool state)
{
  if (state)
    errorStatus = true;
}
