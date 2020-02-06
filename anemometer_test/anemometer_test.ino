int wind_speed;
int count;
unsigned long last_time = 0;
long period = 5000; // ms. anything lower than around 5000 (the response time of anemometer) just creates repeated results.

enum anemometerStates{tx, rx};
enum anemometerStates anemometerState;

const int reply_len = 7; // if query_len is 8 given it has 9 entries...
byte binary [] = {0x00, 0x09, 0x0F};
byte query[] = {0x01, 0x03, 0x00, 0x16, 0x00, 0x01, 0x65, 0xCE};
byte ex_reply[] = {0x01, 0x03, 0x02, 0x00, 0x17, 0xF8, 0x4A}; // wind speed 2.3 m/s
byte reply[reply_len];
const int query_len = sizeof query;

void setup() {
  Serial.begin(9600);
  Serial.println("ready.");

  Serial3.begin(9600);

  anemometerState = tx;
}

unsigned long nextMainExecTime = 0;
void loop() {
  /*
  clearBuffer();
  Serial3.write(query, query_len);
  delay(50); // ms
  
  clearBuffer(); // may cause problem if reply too quick
  Serial3.write(ex_reply, reply_len); // for testing
  delay(50);
  
  if (recvBytesCount(reply, reply_len)) {
    wind_speed = parseWindSpeed(reply);
    Serial.print("wind spd [m/s]: ");
    Serial.println(wind_speed);
    // put SD saving stuff here
  }
  */
  unsigned long now =  millis();
  if (now - nextMainExecTime < 0)
  return; 
  nextMainExecTime = nextMainExecTime + 500;

  anemometerLoopHandler();
}

void anemometerLoopHandler() {
  unsigned long timer;
  switch (anemometerState) {
    case tx:
      Serial.println("tx"); 
      //Serial.println(Serial3.available()); 
      timer = millis();
      Serial.print(last_time);
      Serial.print(" ");
      Serial.println(timer); 
      if (timer - last_time > period) {
        count = 0;
        clearBuffer();
        Serial3.write(query, query_len);
        //Serial3.write(ex_reply, reply_len); // for testing
        last_time = timer;
        
        anemometerState = rx;
      }
      break;
    case rx:
      Serial.println("rx");
      Serial.println(Serial3.available());
      if (Serial3.available() >= query_len + reply_len) {
        Serial3.readBytes(reply, query_len + reply_len);
        wind_speed = parseWindSpeed(reply);
        Serial.print("wind spd [m/s]: ");
        Serial.println(wind_speed/10.0);
        anemometerState = tx;
      }
      break;
    default:
      Serial.print("something is terribly wrong");
  }
}

void clearBuffer() { //uses Serial3. could generalize
  byte b;
  while (Serial3.available() > 0) {
    b = Serial3.read();
  }
}

void recvBytesAndPrint() {
  int b;
  while (Serial3.available()) {
    b = Serial3.read();
    Serial.print(b, HEX);
    Serial.print(' ');
  }
  Serial.println();
}

void recvBytesCount(byte *buf, int len) {
  // returns whether len bytes were read from Serial3
  while (Serial3.available() && count < len) {
    buf[count++] = Serial3.read();
  }
}

int parseWindSpeed(byte *reply) {
  wind_speed = 0.0;
  wind_speed += reply[query_len + 3] * 256; // sent in as if reply[3] were the thous. and hun. digits
  wind_speed += reply[query_len + 4];         //  and reply[4] were tens and ones of a 4-digit hex number
  return wind_speed; // wind speed is in dm/s, ie 23 = 2.3 m/s
}
