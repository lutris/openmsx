// $Id$

#include "MSXFmPac.hh"
#include "Device.hh"
#include "MSXCPU.hh"
#include "CPU.hh"


namespace openmsx {

static const char* PAC_Header = "PAC2 BACKUP DATA";
//                               1234567890123456

MSXFmPac::MSXFmPac(Device* config, const EmuTime& time)
	: MSXDevice(config, time), MSXMusic(config, time), 
	  sram(0x1FFE, config, PAC_Header)
{
	reset(time);
}

MSXFmPac::~MSXFmPac()
{
}

void MSXFmPac::reset(const EmuTime& time)
{
	MSXMusic::reset(time);
	enable = 0;
	sramEnabled = false;
	bank = 0;
	r1ffe = r1fff = 0xFF;	// TODO check
}

void MSXFmPac::writeIO(byte port, byte value, const EmuTime& time)
{
	if (enable & 1) {
		MSXMusic::writeIO(port, value, time);
	}
}

byte MSXFmPac::readMem(word address, const EmuTime& time)
{
	address &= 0x3FFF;
	switch (address) {
		case 0x3FF6:
			return enable;
		case 0x3FF7:
			return bank;
		default:
			if (sramEnabled) {
				if (address < 0x1FFE) {
					return sram.read(address);
				} else if (address == 0x1FFE) {
					return r1ffe;
				} else if (address == 0x1FFF) {
					return r1fff;
				} else {
					return 0xFF;
				}
			} else {
				return rom.read(bank * 0x4000 + address);
			}
	}
}

const byte* MSXFmPac::getReadCacheLine(word address) const
{
	address &= 0x3FFF;
	if (address == (0x3FF6 & CPU::CACHE_LINE_HIGH)) {
		return NULL;
	}
	if (sramEnabled) {
		if (address < (0x1FFE & CPU::CACHE_LINE_HIGH)) {
			return sram.getBlock(address);
		} else if (address == (0x1FFE & CPU::CACHE_LINE_HIGH)) {
			return NULL;
		} else {
			return unmappedRead;
		}
	} else {
		return rom.getBlock(bank * 0x4000 + address);
	}
}

void MSXFmPac::writeMem(word address, byte value, const EmuTime& time)
{
	address &= 0x3FFF;
	switch (address) {
		case 0x1FFE:
			r1ffe = value;
			checkSramEnable();
			break;
		case 0x1FFF:
			r1fff = value;
			checkSramEnable();
			break;
		case 0x3FF4:
			// TODO check if "enable" has any effect
			writeRegisterPort(value, time);
			break;
		case 0x3FF5:
			// TODO check if "enable" has any effect
			writeDataPort(value, time);
			break;
		case 0x3FF6:
			enable = value & 0x11;
			break;
		case 0x3FF7: {
			byte newBank = value & 0x03;
			if (bank != newBank) {
				bank = newBank;
				MSXCPU::instance().invalidateCache(0x0000,
				              0x10000 / CPU::CACHE_LINE_SIZE);
			}
			break;
		}
		default:
			if (sramEnabled && (address < 0x1FFE)) {
				sram.write(address, value);
			}
	}
}

byte* MSXFmPac::getWriteCacheLine(word address) const
{
	address &= 0x3FFF;
	if (address == (0x1FFE & CPU::CACHE_LINE_HIGH)) {
		return NULL;
	}
	if (address == (0x3FF4 & CPU::CACHE_LINE_HIGH)) {
		return NULL;
	}
	if (sramEnabled && (address < 0x1FFE)) {
		return sram.getBlock(address);
	} else {
		return unmappedWrite;
	}
}

void MSXFmPac::checkSramEnable()
{
	bool newEnabled = (r1ffe == 0x4D) && (r1fff == 0x69);
	if (sramEnabled != newEnabled) {
		sramEnabled = newEnabled;
		MSXCPU::instance().invalidateCache(0x0000,
		                              0x10000 / CPU::CACHE_LINE_SIZE);
	}
}

} // namespace openmsx
