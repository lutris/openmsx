// $Id$

#ifndef AY8910_HH
#define AY8910_HH

#include "openmsx.hh"
#include "SoundDevice.hh"
#include "SimpleDebuggable.hh"

namespace openmsx {

class MSXMotherBoard;
class AY8910Periphery;
class XMLElement;
class EmuTime;

/** This class implements the AY-3-8910 sound chip.
  * Only the AY-3-8910 is emulated, no surrounding hardware,
  * use the class AY8910Periphery to connect peripherals.
  */
class AY8910 : private SoundDevice, private SimpleDebuggable
{
public:
	AY8910(MSXMotherBoard& motherBoard, AY8910Periphery& periphery_,
	       const XMLElement& config, const EmuTime& time);
	virtual ~AY8910();

	byte readRegister(byte reg, const EmuTime& time);
	byte peekRegister(byte reg, const EmuTime& time) const;
	void writeRegister(byte reg, byte value, const EmuTime& time);
	void reset(const EmuTime& time);

private:
	class Generator {
	public:
		inline void reset(byte output = 0);
		inline void setPeriod(int value, unsigned int updateStep);
	protected:
		Generator();

		/** Time between output steps.
		  * For tones, this is half the period of the square wave.
		  * For noise, this is the time before the random generator produces
		  * its next output.
		  */
		int period;
		/** Time left in this period: 1 <= count <= period. */
		int count;
		/** Current state of the wave.
		  * For tones, this is 0 or 1.
		  * For noise, this is 0 or 0x38.
		  */
		byte output;
	};

	class ToneGenerator: public Generator {
	public:
		/** Advance this generator in time.
		  * @param duration Length of interval to simulate.
		  * @return Amount of time the generator output is 1 during the given
		  *    time interval.
		  *    This value is only calculated if template parameter "enabled"
		  *    is true, otherwise 0 is returned.
		  */
		template <bool enabled>
		inline int advance(int duration);
	};

	class NoiseGenerator: public Generator {
	public:
		NoiseGenerator();

		inline void reset();
		/** Gets the current output of this noise generator.
		  */
		inline byte getOutput();
		/** Advance noise generator in time, but not past the next output flip.
		  * @param duration Length of interval to simulate.
		  * @return Amount of time actually advanced.
		  *   If no flip occurs, this is equal to the duration parameter.
		  */
		inline int advanceToFlip(int duration);
		/** Advance noise generator in time.
		  * @param duration Length of interval to simulate.
		  */
		inline void advance(int duration);
	private:
		int random;
	};

	class Amplitude {
	public:
		Amplitude();
		inline unsigned int getVolume(byte chan);
		inline void setChannelVolume(byte chan, byte value);
		inline void setEnvelopeVolume(byte volume);
		inline void setMasterVolume(int volume);
		inline bool anyEnvelope();
	private:
		unsigned int volTable[16];
		unsigned int vol[3];
		bool envChan[3];
		unsigned int envVolume;
	};

	class Envelope {
	public:
		inline Envelope(Amplitude& amplitude);
		inline void reset();
		inline void setPeriod(int value, unsigned int updateStep);
		inline void setShape(byte shape);
		inline bool isChanging();
		inline void advance(int duration);
	private:
		/** Gets the current envolope volume.
		  * @return [0..15].
		  */
		inline byte getVolume();

		Amplitude& amplitude;
		int period;
		int count;
		int step;
		int attack;
		bool hold, alternate, holding;
	};

	// SoundDevice:
	virtual const std::string& getName() const;
	virtual const std::string& getDescription() const;
	virtual void setVolume(int volume);
	virtual void setSampleRate(int sampleRate);
	virtual void updateBuffer(unsigned length, int* buffer,
		const EmuTime& time, const EmuDuration& sampDur);

	// SimpleDebuggable
	virtual byte read(unsigned address, const EmuTime& time);
	virtual void write(unsigned address, byte value, const EmuTime& time);

	void wrtReg(byte reg, byte value, const EmuTime& time);
	void checkMute();

	AY8910Periphery& periphery;
	unsigned int updateStep;
	byte regs[16];
	byte oldEnable;
	ToneGenerator tone[3];
	NoiseGenerator noise;
	Envelope envelope;
	Amplitude amplitude;
};

} // namespace openmsx

#endif // AY8910_HH
