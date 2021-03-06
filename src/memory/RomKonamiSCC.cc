// KONAMI 8kB cartridges with SCC
//
// this type is used by Konami cartridges that do have an SCC and some others
// examples of cartridges: Nemesis 2, Nemesis 3, King's Valley 2, Space Manbow
// Solid Snake, Quarth, Ashguine 1, Animal, Arkanoid 2, ...
// Those last 3 were probably modified ROM images, they should be ASCII8
//
// The address to change banks:
//  bank 1: 0x5000 - 0x57ff (0x5000 used)
//  bank 2: 0x7000 - 0x77ff (0x7000 used)
//  bank 3: 0x9000 - 0x97ff (0x9000 used)
//  bank 4: 0xB000 - 0xB7ff (0xB000 used)

#include "RomKonamiSCC.hh"
#include "SCC.hh"
#include "CacheLine.hh"
#include "Rom.hh"
#include "MSXMotherBoard.hh"
#include "CliComm.hh"
#include "serialize.hh"
#include "memory.hh"

namespace openmsx {

RomKonamiSCC::RomKonamiSCC(const DeviceConfig& config, std::unique_ptr<Rom> rom_)
	: Rom8kBBlocks(config, std::move(rom_))
	, scc(make_unique<SCC>("SCC", config, getCurrentTime()))
{

	// warn if a ROM is used that would not work on a real KonamiSCC mapper
	if (rom->getSize() > 512 * 1024) {
		getMotherBoard().getMSXCliComm().printWarning("The size of "
				"this ROM image is larger than 512kB, which is "
				"not supported on real Konami SCC mapper "
				"chips!"); }

	powerUp(getCurrentTime());
}

RomKonamiSCC::~RomKonamiSCC()
{
}

void RomKonamiSCC::powerUp(EmuTime::param time)
{
	scc->powerUp(time);
	reset(time);
}

void RomKonamiSCC::reset(EmuTime::param time)
{
	setUnmapped(0);
	setUnmapped(1);
	for (int i = 2; i < 6; i++) {
		setRom(i, i - 2);
	}
	setUnmapped(6);
	setUnmapped(7);

	sccEnabled = false;
	scc->reset(time);
}

byte RomKonamiSCC::peekMem(word address, EmuTime::param time) const
{
	if (sccEnabled && (0x9800 <= address) && (address < 0xA000)) {
		return scc->peekMem(address & 0xFF, time);
	} else {
		return Rom8kBBlocks::peekMem(address, time);
	}
}

byte RomKonamiSCC::readMem(word address, EmuTime::param time)
{
	if (sccEnabled && (0x9800 <= address) && (address < 0xA000)) {
		return scc->readMem(address & 0xFF, time);
	} else {
		return Rom8kBBlocks::readMem(address, time);
	}
}

const byte* RomKonamiSCC::getReadCacheLine(word address) const
{
	if (sccEnabled && (0x9800 <= address) && (address < 0xA000)) {
		// don't cache SCC
		return nullptr;
	} else {
		return Rom8kBBlocks::getReadCacheLine(address);
	}
}

void RomKonamiSCC::writeMem(word address, byte value, EmuTime::param time)
{
	if ((address < 0x5000) || (address >= 0xC000)) {
		return;
	}
	if (sccEnabled && (0x9800 <= address) && (address < 0xA000)) {
		// write to SCC
		scc->writeMem(address & 0xFF, value, time);
		return;
	}
	if ((address & 0xF800) == 0x9000) {
		// SCC enable/disable
		sccEnabled = ((value & 0x3F) == 0x3F);
		invalidateMemCache(0x9800, 0x0800);
	}
	if ((address & 0x1800) == 0x1000) {
		// page selection
		setRom(address >> 13, value);
	}
}

byte* RomKonamiSCC::getWriteCacheLine(word address) const
{
	if ((address < 0x5000) || (address >= 0xC000)) {
		return unmappedWrite;
	} else if (sccEnabled && (0x9800 <= address) && (address < 0xA000)) {
		// write to SCC
		return nullptr;
	} else if ((address & 0xF800) == (0x9000 & CacheLine::HIGH)) {
		// SCC enable/disable
		return nullptr;
	} else if ((address & 0x1800) == (0x1000 & CacheLine::HIGH)) {
		// page selection
		return nullptr;
	} else {
		return unmappedWrite;
	}
}

template<typename Archive>
void RomKonamiSCC::serialize(Archive& ar, unsigned /*version*/)
{
	ar.template serializeBase<Rom8kBBlocks>(*this);
	ar.serialize("scc", *scc);
	ar.serialize("sccEnabled", sccEnabled);
}
INSTANTIATE_SERIALIZE_METHODS(RomKonamiSCC);
REGISTER_MSXDEVICE(RomKonamiSCC, "RomKonamiSCC");

} // namespace openmsx
