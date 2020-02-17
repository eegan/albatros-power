int dividerPin = A0;
int dividershuntPin = A1; // TODO make it so this just gets the val. from shuntPin
float dividershuntVREF = 1100; // TODO make it so this just gets the val. from shuntPin
const int dividerVMAX = 31100; // in mV. voltage across input of divider needed to get analogRef volts across output
//const int divider_yint = -200; // in mV. Empirically determined y int of (Vreal, Vmeas) linear fit.
const int divider_yint = 0;
int voltage;                   
int divider_reading;
float shunt_voltage;

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
  shunt_voltage = (analogRead(dividershuntPin) / 1024.) * (dividershuntVREF);
  return (divider_reading / 1024.) * (dividerVMAX) - shunt_voltage - divider_yint;
}
