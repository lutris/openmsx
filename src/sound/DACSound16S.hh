// This class implements a 16 bit signed DAC

#ifndef DACSOUND16S_HH
#define DACSOUND16S_HH

#include "SoundDevice.hh"
#include "BlipBuffer.hh"
#include <cstdint>

namespace openmsx {

class DACSound16S : public SoundDevice
{
public:
	DACSound16S(string_ref name, string_ref desc,
	            const DeviceConfig& config);
	virtual ~DACSound16S();

	void reset(EmuTime::param time);
	void writeDAC(int16_t value, EmuTime::param time);

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	// SoundDevice
	virtual void setOutputRate(unsigned sampleRate);
	virtual void generateChannels(int** bufs, unsigned num);
	virtual bool updateBuffer(unsigned length, int* buffer,
	                          EmuTime::param time);

	BlipBuffer blip;
	int16_t lastWrittenValue;
};

} // namespace openmsx

#endif
