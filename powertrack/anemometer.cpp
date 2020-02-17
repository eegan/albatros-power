#include "Arduino.h"
#include "anemometer.h"

// TODO: clean up length hard-coding if possible
byte anemometer::binary [3] = {0x00, 0x09, 0x0F};
byte anemometer::query[8] = {0x01, 0x03, 0x00, 0x16, 0x00, 0x01, 0x65, 0xCE};
byte anemometer::ex_reply[7] = {0x01, 0x03, 0x02, 0x00, 0x17, 0xF8, 0x4A}; // wind speed 2.3 m/s

void anemometer::init()
{
	Port.begin(9600);
	anemometerState = tx;
	sampleAvailableFlag = false;
}

void anemometer::loopHandler()
{
  switch (anemometerState) {
    case tx:
      anemom_timer = millis();
      if (anemom_timer - anemom_last_time > anemom_period) {
        clearBuffer();
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
        wind_speed = parseWindSpeed(reply);
		sampleAvailableFlag = true;
        anemometerState = tx;
      } else if (anemom_timer - anemom_last_time > anemom_timeout) {
        anemometerState = tx;
      }
      break;
    default:
      return;
  }
}

void anemometer::clearBuffer() { //uses Port. could generalize
  byte b;
  while (Port.available() > 0) {
    b = Port.read();
  }
}

int anemometer::parseWindSpeed(byte *reply) {
  wind_speed = 0.0;
  wind_speed += reply[query_len + 3] * 256; // sent in as if reply[3] were the thous. and hun. digits
  wind_speed += reply[query_len + 4];         //  and reply[4] were tens and ones of a 4-digit hex number
  return wind_speed; // wind speed is in dm/s, ie 23 = 2.3 m/s
}

bool anemometer::sampleAvailable()
{
	return sampleAvailableFlag;
}

float anemometer::result()
{
	sampleAvailableFlag = false;
	return wind_speed;
}