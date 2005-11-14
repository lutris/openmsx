// $Id$

#include "Y8950KeyboardDevice.hh"

namespace openmsx {

const std::string &Y8950KeyboardDevice::getClass() const
{
	static const std::string className("Y8950 Keyboard Port");
	return className;
}

} // namespace openmsx
