#include "anemometer.h"

static uint16_t hAnemom1LogVar;
//static uint16_t hAnemom2LogVar;

anemometer an1 = anemometer(Serial3);
anemometer an2 = anemometer(Serial2);

void anemometerInit() {
	an1.init();
	hAnemom1LogVar = loggerRegisterLogVariable("Wind Speed [dm/s]", lvtNumeric);
	an2.init();
	hAnemom2LogVar = loggerRegisterLogVariable("Wind Speed 2 [dm/s]", lvtNumeric);
}

void anemometerLoopHandler() {
	an1.loopHandler();
	if (an1.sampleAvailable())
		loggerLogSample(hAnemom1LogVar, (int)an1.result());
	an2.loopHandler();
	if (an2.sampleAvailable())
		loggerLogSample(hAnemom2LogVar, (int)an2.result());
}
