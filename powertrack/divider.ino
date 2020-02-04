int dividerPin = A0;
const int dividerVMAX = 31100; // in mV. voltage across input of divider needed to get analogRef volts across output
const int divider_yint = -200; // in mV. Empirically determined y int of (Vreal, Vmeas) linear fit.
int voltage;                   
int divider_reading;

unsigned long divider_timer;
long divider_last_time = 0;
int divider_period = 1000; // ms between readings

int hDividerLogVar;

void dividerInit() {
  analogReference(INTERNAL1V1);
  hDividerLogVar = loggerRegisterLogVariable("Voltage [mV]", lvtNumeric);
}

void dividerLoopHandler() {
  divider_timer = millis();
  if (divider_timer - divider_last_time > divider_period) {
    voltage = dividerGetVoltage();
    loggerLogSample(hDividerLogVar, voltage);
    divider_last_time = divider_timer;
  }
}

int dividerGetVoltage() {
  // returns voltage in mV
  divider_reading = analogRead(dividerPin);
  return (divider_reading / 1024.) * (dividerVMAX) - divider_yint;
}
