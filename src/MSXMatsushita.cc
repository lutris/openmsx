// $Id$

#include "MSXMatsushita.hh"


const byte ID = 0x08;

MSXMatsushita::MSXMatsushita(MSXConfig::Device *config, const EmuTime &time)
	: MSXDevice(config, time), MSXSwitchedDevice(ID),
	  sram(0x800, config)
{
	// TODO find out what ports 0x41 0x45 0x46 are used for
	//      (and if they belong to this device)

	reset(time);
}

MSXMatsushita::~MSXMatsushita()
{
}

void MSXMatsushita::reset(const EmuTime &time)
{
	address = 0;	// TODO check this
}

byte MSXMatsushita::readIO(byte port, const EmuTime &time)
{
	// TODO: Port 7 and 8 can be read as well.
	byte result;
	switch (port & 0x0F) {
	case 0:
		result = ~ID;
		break;
	case 3:
		result = (((pattern & 0x80) ? color2 : color1) << 4)
		        | ((pattern & 0x40) ? color2 : color1);
		pattern = (pattern << 2) | (pattern >> 6);
		break;
	case 9:
		result = sram.read(address);
		address = (address + 1) & 0x7FF;
		break;
	default:
		result = 0xFF;
	}
	return result;
}

void MSXMatsushita::writeIO(byte port, byte value, const EmuTime &time)
{
	switch (port & 0x0F) {
	case 3:
		color2 = (value & 0xF0) >> 4;
		color1 =  value & 0x0F;
		break;
	case 4:
		pattern = value;
		break;
	case 7:
		// set address (low)
		address = (address & 0xFF00) | value;
		break;
	case 8:
		// set address (high)
		address = (address & 0x00FF) | ((value & 0x07) << 8);
		break;
	case 9:
		// write sram
		sram.write(address, value);
		address = (address + 1) & 0x7FF;
		break;
	}
}
