static int wind_speed;
static unsigned long anemom_timer;
static unsigned long anemom_last_time = 0;
static long anemom_period = 5000; // ms. anything lower than around 5000 (the response time of anemometer) just creates repeated results.
static long anemom_timeout = 10000; //ms of silence from anemometer before another query sent

static HardwareSerial &Port = Serial3;
static enum anemometerStates{tx, rx};
static enum anemometerStates anemometerState;

static byte binary [] = {0x00, 0x09, 0x0F};
static byte query[] = {0x01, 0x03, 0x00, 0x16, 0x00, 0x01, 0x65, 0xCE};
static const int query_len = sizeof query;
static byte ex_reply[] = {0x01, 0x03, 0x02, 0x00, 0x17, 0xF8, 0x4A}; // wind speed 2.3 m/s
static const int reply_len = sizeof ex_reply;
static byte reply[query_len + reply_len]; // have to catch the loopback too

static uint16_t hAnemomLogVar;

void anemometer2_Init() {
  Port.begin(9600);
  hAnemomLogVar = loggerRegisterLogVariable("Wind Speed [dm/s]", lvtNumeric);
  anemometerState = tx;
}

void anemometer2_LoopHandler() {
  switch (anemometerState) {
    case tx:
      anemom_timer = millis();
      if (anemom_timer - anemom_last_time > anemom_period) {
        anemom2_ClearBuffer();
        Port.write(query, query_len);
        //Port.write(ex_reply, reply_len); // for testing
        anemom_last_time = anemom_timer;
        
        anemometerState = rx;
      }
      break;
    case rx:
      anemom_timer = millis();
      if (Port.available() >= query_len + reply_len) {
        Port.readBytes(reply, query_len + reply_len);
        wind_speed = anemometer2_parseWindSpeed(reply);
        loggerLogSample(hAnemomLogVar, wind_speed);
        anemometerState = tx;
      } else if (anemom_timer - anemom_last_time > anemom_timeout) {
        anemometerState = tx;
      }
      break;
    default:
      return;
  }
}

void anemom2_ClearBuffer() { //uses Port. could generalize
  byte b;
  while (Port.available() > 0) {
    b = Port.read();
  }
}

int anemometer2_parseWindSpeed(byte *reply) {
  wind_speed = 0.0;
  wind_speed += reply[query_len + 3] * 256; // sent in as if reply[3] were the thous. and hun. digits
  wind_speed += reply[query_len + 4];         //  and reply[4] were tens and ones of a 4-digit hex number
  return wind_speed; // wind speed is in dm/s, ie 23 = 2.3 m/s
}
