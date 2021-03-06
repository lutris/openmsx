#ifndef VDPCMDENGINE_HH
#define VDPCMDENGINE_HH

#include "VDP.hh"
#include "VDPAccessSlots.hh"
#include "serialize_meta.hh"
#include "openmsx.hh"
#include "noncopyable.hh"
#include "likely.hh"
#include <memory>

namespace openmsx {

class VDPVRAM;
class DisplayMode;
class CommandController;
class RenderSettings;
class BooleanSetting;
class TclCallback;


/** This is an abstract base class the VDP commands
  */
class VDPCmd {
public:
	virtual ~VDPCmd() {}

	/** Prepare execution of cmd
	  */
	virtual void start(EmuTime::param time, VDPCmdEngine& engine) = 0;

	/** Perform a given V9938 graphical operation.
	  */
	virtual void execute(EmuTime::param limit, VDPCmdEngine& engine) = 0;
};


/** VDP command engine by Alex Wulms.
  * Implements command execution unit of V9938/58.
  */
class VDPCmdEngine : private noncopyable
{
public:
	VDPCmdEngine(VDP& vdp, RenderSettings& renderSettings_,
	             CommandController& commandController);
	virtual ~VDPCmdEngine();

	/** Reinitialise Renderer state.
	  * @param time The moment in time the reset occurs.
	  */
	void reset(EmuTime::param time);

	/** Synchronises the command engine with the VDP.
	  * Ideally this would be a private method, but the current
	  * design doesn't allow that.
	  * @param time The moment in emulated time to sync to.
	  */
	inline void sync(EmuTime::param time) {
		if (!currentCommand) return;
		currentCommand->execute(time, *this);
		if (currentCommand && unlikely(vdp.cpuAccessScheduled())) {
			// If there's a CPU access scheduled, then the next
			// slot will be used by the CPU. So we take a later
			// slot.
			nextAccessSlot(VDPAccessSlots::DELTA_1); // skip one slot
			assert(this->time > time);
		}
	}

	/** Gets the command engine status (part of S#2).
	  * Bit 7 (TR) is set when the command engine is ready for
	  * a pixel transfer.
	  * Bit 4 (BD) is set when the boundary color is detected.
	  * Bit 0 (CE) is set when a command is in progress.
	  */
	inline byte getStatus(EmuTime::param time) {
		if (time >= statusChangeTime) {
			sync(time);
		}
		return status;
	}

	/** Use this method to transfer pixel(s) from VDP to CPU.
	  * This method implements V9938 S#7.
	  * @param time The moment in emulated time this read occurs.
	  * @return Color value of the pixel.
	  */
	inline byte readColor(EmuTime::param time) {
		sync(time);
		return COL;
	}
	inline void resetColor() {
		// Note: Real VDP always resets TR, but for such a short time
		//       that the MSX won't notice it.
		// TODO: What happens on non-transfer commands?
		if (!currentCommand) status &= 0x7F;
		transfer = true;
	}

	/** Gets the X coordinate of a border detected by SRCH (intended behaviour,
          * as documented in the V9938 technical data book). However, real VDP
          * simply returns the current value of the ASX 'temporary source X' counter,
          * regardless of the command that is being executed or was executed most
          * recently
	  * @param time The moment in emulated time this get occurs.
	  */
	inline unsigned getBorderX(EmuTime::param time) {
		sync(time);
		return ASX;
	}

	/** Writes to a command register.
	  * @param index The register [0..14] to write to.
	  * @param value The new value for the specified register.
	  * @param time The moment in emulated time this write occurs.
	  */
	void setCmdReg(byte index, byte value, EmuTime::param time);

	/** Read the content of a command register. This method is meant to
	  * be used by the debugger, there is no strict guarantee that the
	  * returned value is the correct value at _exactly_ this moment in
	  * time (IOW this method does not sync the complete CmdEngine)
	  * @param index The register [0..14] to read from.
	  */
	byte peekCmdReg(byte index);

	/** Informs the command engine of a VDP display mode change.
	  * @param mode The new display mode.
	  * @param time The moment in emulated time this change occurs.
	  */
	void updateDisplayMode(DisplayMode mode, EmuTime::param time);

	/** Interface for logical operations.
	  */
	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	template <template <typename Mode> class Command>
	void createHEngines(unsigned cmd);
	void deleteHEngines(unsigned cmd);

	template <template <typename Mode, typename LogOp> class Command>
	void createLEngines(unsigned cmd, VDPCmd* dummy);
	void deleteLEngines(unsigned cmd);

	void executeCommand(EmuTime::param time);

	inline void nextAccessSlot() {
		time = vdp.getAccessSlot(time, VDPAccessSlots::DELTA_0);
	}
	inline void nextAccessSlot(VDPAccessSlots::Delta delta) {
		time = vdp.getAccessSlot(time, delta);
	}
	inline VDPAccessSlots::Calculator getSlotCalculator(
			EmuTime::param limit) const {
		return vdp.getAccessSlotCalculator(time, limit);
	}

	/** Finshed executing graphical operation.
	  */
	void commandDone(EmuTime::param time);

	/** Report the VDP command specified in the registers.
	  */
	void reportVdpCommand();


	/** The VDP this command engine is part of.
	  */
	VDP& vdp;
	VDPVRAM& vram;

	RenderSettings& renderSettings;

	/** Only call reportVdpCommand() when this setting is turned on
	  */
	const std::unique_ptr<BooleanSetting> cmdTraceSetting;
	const std::unique_ptr<TclCallback> cmdInProgressCallback;

	VDPCmd* commands[256][4];
	VDPCmd* currentCommand;

	/** Time at which the next operation cycle starts.
	  * A cycle consists of reading source VRAM (if applicable),
	  * reading destination VRAM (if applicable),
	  * writing destination VRAM and updating coordinates.
	  * For perfect timing each phase within a cycle should be timed
	  * explicitly, but for now we use an average execution time per
	  * cycle.
	  */
	EmuTime time;

	/** Lower bound for the time when the status register will change, IOW
	  * the status register will not change before this time.
	  * Can also be EmuTime::zero -> status can change any moment
	  * or EmuTime::infinity -> this command doesn't change the status
	  */
	EmuTime statusChangeTime;

	/** Some commands execute multiple VRAM accesses per pixel
	  * (e.g. LMMM does two reads and a write). This variable keeps
	  * track of where in the (sub)command we are. */
	int phase;

	/** Current screen mode.
	  * 0 -> SCREEN5, 1 -> SCREEN6, 2 -> SCREEN7, 3 -> SCREEN8,
	  * -1 -> other.
	  */
	int scrMode;

	/** VDP command registers.
	  */
	unsigned SX, SY, DX, DY, NX, NY; // registers that can be set by CPU
	unsigned ASX, ADX, ANX; // Temporary registers used in the VDP commands
                                // Register ASX can be read (via status register 8/9)
	byte COL, ARG, CMD;

	/** When a command needs multiple VRAM accesses per pixel, the result
	 * of intermediate reads is stored in these variables. */
	byte tmpSrc;
	byte tmpDst;

	/** The command engine status (part of S#2).
	  * Bit 7 (TR) is set when the command engine is ready for
	  * a pixel transfer.
	  * Bit 4 (BD) is set when the boundary color is detected.
	  * Bit 0 (CE) is set when a command is in progress.
	  */
	byte status;

	/** Used in LMCM LMMC HMMC cmds, true when CPU has read or written
	  * next byte.
	  */
	bool transfer;

	/** Flag that indicated whether extended VRAM is available
	 */
	const bool hasExtendedVRAM;

	friend struct AbortCmd;
	friend struct PointBaseCmd;
	friend struct PsetBaseCmd;
	friend struct SrchBaseCmd;
	friend struct LineBaseCmd;
	friend class BlockCmd;
	template<typename> friend struct PointCmd;
	template<typename> friend struct SrchCmd;
	template<typename> friend struct LmcmCmd;
	template<typename> friend struct HmmvCmd;
	template<typename> friend struct HmmmCmd;
	template<typename> friend struct YmmmCmd;
	template<typename> friend struct HmmcCmd;
	template<typename> friend struct LmmvBaseCmd;
	template<typename> friend struct LmmmBaseCmd;
	template<typename> friend struct LmmcBaseCmd;
	template<typename, typename> friend struct PsetCmd;
	template<typename, typename> friend struct LineCmd;
	template<typename, typename> friend struct LmmvCmd;
	template<typename, typename> friend struct LmmmCmd;
	template<typename, typename> friend struct LmmcCmd;
};
SERIALIZE_CLASS_VERSION(VDPCmdEngine, 3);

} // namespace openmsx

#endif
