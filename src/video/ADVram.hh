#ifndef ADVRAM_HH
#define ADVRAM_HH

#include "MSXDevice.hh"

namespace openmsx {

class VDP;
class VDPVRAM;

/** Implementation of direct cpu access to VRAM.  ADVram (Accesso
  * Direito &agrave; Vram is a rare hardware modification that allows the
  * CPU to access the video ram in the same way as ordinary ram.
  */
class ADVram : public MSXDevice
{
public:
	explicit ADVram(const DeviceConfig& config);

	/** This method is called on reset.  Reset the mapper register and
	 * the planar bit, if the device is configured with an enable bit
	 * then that bit is reset as well.
	 */
	virtual void reset(EmuTime::param time);

	/** Read a byte from an IO port, change mode bits.  The planar bit
	 * and possibly the enable bit are set according to address lines
	 * that are normally ignored for IO reads.  Returns 255.
	 */
	virtual byte readIO(word port, EmuTime::param time);
	// default peekIO() implementation is ok.

	/** Write a byte to a given IO port, set mapper register.  */
	virtual void writeIO(word port, byte value, EmuTime::param time);

	/** Read a byte from a location in the video ram at a certain
	 * time.  If the device is enabled then the value returned comes
	 * from the video ram, otherwise it returns 255.
	 */
	virtual byte readMem(word address, EmuTime::param time);

	/** Write a given byte at a certain time to a given location in
	 * the video ram.  If the device is enabled then the write is
	 * redirected to the video ram, if it is not, nothing happens.
	 */
	virtual void writeMem(word address, byte value, EmuTime::param time);

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	virtual void init();
	inline unsigned calcAddress(word address) const;

	VDP* vdp;
	VDPVRAM* vram;
	/** Bit mask applied to logical addresses, reflects vram size.  */
	/*const*/ unsigned mask;
	/** Base address of selected page in vram.  */
	unsigned baseAddr;
	/** True if ADVram device is enabled.  */
	bool enabled;
	/** True if ADVram device can be switched on or off by performing
		IO-reads.  */
	const bool hasEnable;
	/** True if address bits are shuffled as is appropriate for screen
		7 and higher.  */
	bool planar;
};

} // namespace openmsx

#endif // ADVRAM_HH
