// $Id$

#include <iostream>
#include "Leds.hh"
#include "openmsx.hh"
#include "CliCommOutput.hh"


namespace openmsx {

Leds::Leds()
	: output(CliCommOutput::instance())
{
	pwrLed = capsLed = kanaLed = pauseLed = turboLed = true;
	fddLedCounter=1;
	setLed(POWER_OFF);
	setLed(CAPS_OFF);
	setLed(KANA_OFF);
	setLed(PAUSE_OFF);
	setLed(TURBO_OFF);
	setLed(FDD_OFF);
}

Leds::~Leds()
{
	setLed(POWER_OFF);
	setLed(CAPS_OFF);
	setLed(KANA_OFF);
	setLed(PAUSE_OFF);
	setLed(TURBO_OFF);
	if (fddLedCounter>0) setLed(FDD_OFF);
}

Leds& Leds::instance()
{
	static Leds oneInstance;
	return oneInstance;
}


void Leds::setLed(LEDCommand led)
{
	switch (led) {
	case POWER_ON:
		if (!pwrLed) {
			output.printUpdate("Power LED ON");
			pwrLed = true;
		}
		break;
	case POWER_OFF:
		if (pwrLed) {
			output.printUpdate("Power LED OFF");
			pwrLed = false;
		}
		break;
	case CAPS_ON:
		if (!capsLed) {
			output.printUpdate("Caps LED ON");
			capsLed = true;
		}
		break;
	case CAPS_OFF:
		if (capsLed) {
			output.printUpdate("Caps LED OFF");
			capsLed = false;
		}
		break;
	case KANA_ON:
		if (!kanaLed) {
			output.printUpdate("Kana LED ON");
			kanaLed = true;
		}
		break;
	case KANA_OFF:
		if (kanaLed) {
			output.printUpdate("Kana LED OFF");
			kanaLed = false;
		}
		break;
	case PAUSE_ON:
		if (!pauseLed) {
			output.printUpdate("Pause LED ON");
			pauseLed = true;
		}
		break;
	case PAUSE_OFF:
		if (pauseLed) {
			output.printUpdate("Pause LED OFF");
			pauseLed = false;
		}
		break;
	case TURBO_ON:
		if (!turboLed) {
			output.printUpdate("Turbo LED ON");
			turboLed = true;
		}
		break;
	case TURBO_OFF:
		if (turboLed) {
			output.printUpdate("Turbo LED OFF");
			turboLed = false;
		}
		break;
	case FDD_ON:
		if (fddLedCounter==0) { // turn on if it was off
			output.printUpdate("FDD LED ON");
		}
		fddLedCounter++;
		break;
	case FDD_OFF:
		if (fddLedCounter==1) { // only turn off when it is the last one
			output.printUpdate("FDD LED OFF");
		}
		fddLedCounter--;
		break;
	}
}

} // namespace openmsx
