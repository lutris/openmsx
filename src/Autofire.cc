#include "Autofire.hh"
#include "IntegerSetting.hh"
#include "memory.hh"
#include <algorithm>
#include <cassert>

namespace openmsx {

Autofire::Autofire(CommandController& commandController,
                   unsigned newMinInts, unsigned newMaxInts, string_ref name)
	: min_ints(std::max(newMinInts, 1u))
	, max_ints(std::max(newMaxInts, min_ints + 1))
	, speedSetting(make_unique<IntegerSetting>(commandController, name,
		"controls the speed of this autofire circuit", 0, 0, 100))
	, clock(EmuTime::zero)
{
	setClock();
	speedSetting->attach(*this);
}

Autofire::~Autofire()
{
	speedSetting->detach(*this);
}

void Autofire::setClock()
{
	int speed = speedSetting->getInt();
	clock.setFreq(
	    (2 * 50 * 60) / (max_ints - (speed * (max_ints - min_ints)) / 100));
}

void Autofire::update(const Setting& setting)
{
	(void)setting;
	assert(&setting == speedSetting.get());
	setClock();
}

bool Autofire::getSignal(EmuTime::param time)
{
	return speedSetting->getInt() == 0 ?
		false : clock.getTicksTill(time) & 1;
}

} // namespace openmsx
