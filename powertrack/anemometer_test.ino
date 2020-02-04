int wind_speed;
unsigned long anemom_timer;
unsigned long anemom_last_time = 0;
long anemom_period = 5000; // ms. anything lower than around 5000 (the response time of anemometer) just creates repeated results.

enum anemometerStates{tx, rx};
enum anemometerStates anemometerState;

const int reply_len = 7; // if query_len is 8 given it has 9 entries...
byte binary [] = {0x00, 0x09, 0x0F};
byte query[] = {0x01, 0x03, 0x00, 0x16, 0x00, 0x01, 0x65, 0xCE};
byte ex_reply[] = {0x01, 0x03, 0x02, 0x00, 0x17, 0xF8, 0x4A}; // wind speed 2.3 m/s
byte reply[reply_len];
const int query_len = sizeof query;

uint16_t hAnemomLogVar;

void anemometerInit() {
  Serial3.begin(9600);
  hAnemomLogVar = loggerRegisterLogVariable("Wind Speed [dm/s]", lvtNumeric);
  anemometerState = tx;
}

void anemometerLoopHandler() {
  switch (anemometerState) {
    case tx:
      anemom_timer = millis();
      if (anemom_timer - anemom_last_time > anemom_period) {
        clearBuffer();
        Serial3.write(query, query_len);
        //Serial3.write(ex_reply, reply_len); // for testing
        anemom_last_time = anemom_timer;
        
        anemometerState = rx;
      }
      break;
    case rx:
      if (Serial3.available() >= query_len + reply_len) {
        Serial3.readBytes(reply, query_len + reply_len);
        wind_speed = parseWindSpeed(reply);
        loggerLogSample(hAnemomLogVar, wind_speed);
        anemometerState = tx;
      }
      break;
    default:
      return;
  }
}

void clearBuffer() { //uses Serial3. could generalize
  byte b;
  while (Serial3.available() > 0) {
    b = Serial3.read();
  }
}

int parseWindSpeed(byte *reply) {
  wind_speed = 0.0;
  wind_speed += reply[query_len + 3] * 256; // sent in as if reply[3] were the thous. and hun. digits
  wind_speed += reply[query_len + 4];         //  and reply[4] were tens and ones of a 4-digit hex number
  return wind_speed; // wind speed is in dm/s, ie 23 = 2.3 m/s
}
