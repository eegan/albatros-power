int shuntPin = A1;
const float shuntResistance = 25; // in mOhm
const float shuntVREF = 1100; // in mV
int current;

unsigned long shunt_timer;
long shunt_last_time = 0;
int shunt_period = 1000; // ms between readings

int hShuntLogVar;

void shuntInit() {
  analogReference(INTERNAL1V1);
  hShuntLogVar = loggerRegisterLogVariable("Current [A]", lvtNumeric);
}

void shuntLoopHandler() {
  shunt_timer = millis();
  if (shunt_timer - shunt_last_time > shunt_period) {
    current = shuntGetCurrent();
    loggerLogSample(hShuntLogVar, current);
    shunt_last_time = shunt_timer;
  }
}

int shuntGetCurrent() {
  // returns current in mA
  int shunt_reading = analogRead(shuntPin);
  return (shunt_reading / 1024.) * (shuntVREF / shuntResistance) * 1000;
}
