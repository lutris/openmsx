#ifndef MOUSE_HH
#define MOUSE_HH

#include "JoystickDevice.hh"
#include "MSXEventListener.hh"
#include "StateChangeListener.hh"
#include "serialize_meta.hh"

namespace openmsx {

class MSXEventDistributor;
class StateChangeDistributor;

class Mouse : public JoystickDevice, private MSXEventListener
            , private StateChangeListener
{
public:
	Mouse(MSXEventDistributor& eventDistributor,
	      StateChangeDistributor& stateChangeDistributor);
	virtual ~Mouse();

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	// Pluggable
	virtual const std::string& getName() const;
	virtual string_ref getDescription() const;
	virtual void plugHelper(Connector& connector, EmuTime::param time);
	virtual void unplugHelper(EmuTime::param time);

	// JoystickDevice
	virtual byte read(EmuTime::param time);
	virtual void write(byte value, EmuTime::param time);

	// MSXEventListener
	virtual void signalEvent(const std::shared_ptr<const Event>& event,
	                         EmuTime::param time);
	// StateChangeListener
	virtual void signalStateChange(const std::shared_ptr<StateChange>& event);
	virtual void stopReplay(EmuTime::param time);

	void createMouseStateChange(EmuTime::param time,
		int deltaX, int deltaY, byte press, byte release);
	void emulateJoystick();
	void plugHelper2();

	MSXEventDistributor& eventDistributor;
	StateChangeDistributor& stateChangeDistributor;
	EmuTime lastTime;
	int phase;
	int xrel, yrel;         // latched X/Y values, these are returned to the MSX
	int curxrel, curyrel;   // running X/Y values, already scaled down
	int absHostX, absHostY; // running X/Y values, not yet scaled down
	byte status;
	bool mouseMode;
};
SERIALIZE_CLASS_VERSION(Mouse, 4);

} // namespace openmsx

#endif
