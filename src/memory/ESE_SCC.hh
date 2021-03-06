#ifndef ESE_SCC_HH
#define ESE_SCC_HH

#include "MSXDevice.hh"
#include <memory>

namespace openmsx {

class SRAM;
class SCC;
class MB89352;
class RomBlockDebuggable;

class ESE_SCC : public MSXDevice
{
public:
	ESE_SCC(const DeviceConfig& config, bool withSCSI);
	virtual ~ESE_SCC();

	virtual void powerUp(EmuTime::param time);
	virtual void reset(EmuTime::param time);

	virtual byte readMem(word address, EmuTime::param time);
	virtual byte peekMem(word address, EmuTime::param time) const;
	virtual void writeMem(word address, byte value, EmuTime::param time);
	virtual const byte* getReadCacheLine(word start) const;
	virtual byte* getWriteCacheLine(word start) const;

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	void setMapperLow(unsigned page, byte value);
	void setMapperHigh(byte value);

	const std::unique_ptr<SRAM> sram;
	const std::unique_ptr<SCC> scc;
	const std::unique_ptr<MB89352> spc;
	const std::unique_ptr<RomBlockDebuggable> romBlockDebug;

	const byte mapperMask;
	byte mapper[4];
	bool spcEnable;
	bool sccEnable;
	bool writeEnable;
};

} // namespace openmsx

#endif
