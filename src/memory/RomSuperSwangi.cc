#include "RomSuperSwangi.hh"
#include "Rom.hh"
#include "CacheLine.hh"
#include "serialize.hh"

namespace openmsx {

RomSuperSwangi::RomSuperSwangi(const DeviceConfig& config, std::unique_ptr<Rom> rom)
	: Rom16kBBlocks(config, std::move(rom))
{
	reset(EmuTime::dummy());
}

void RomSuperSwangi::reset(EmuTime::param /*time*/)
{
	setUnmapped(0);
	setRom(1, 0);
	setRom(2, 0);
	setUnmapped(3);
}

void RomSuperSwangi::writeMem(word address, byte value, EmuTime::param /*time*/)
{
	if (address == 0x8000) {
		setRom(2, value >> 1);
	}
}

byte* RomSuperSwangi::getWriteCacheLine(word address) const
{
	if (address == (0x8000 & CacheLine::HIGH)) return nullptr;
	return unmappedWrite;
}

REGISTER_MSXDEVICE(RomSuperSwangi, "RomSuperSwangi");

} // namespace openmsx
