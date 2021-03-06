#ifndef EVENTDELAY_HH
#define EVENTDELAY_HH

#include "EventListener.hh"
#include "Schedulable.hh"
#include "EmuTime.hh"
#include "build-info.hh"
#include <vector>
#include <deque>
#include <memory>
#include <cstdint>

namespace openmsx {

class Scheduler;
class CommandController;
class Event;
class EventDistributor;
class MSXEventDistributor;
class ReverseManager;
class FloatSetting;

/** This class is responsible for translating host events into MSX events.
  * It also translates host event timing into EmuTime. To better do this
  * we introduce a small delay (default 0.03s) in this translation.
  */
class EventDelay : private EventListener, private Schedulable
{
public:
	EventDelay(Scheduler& scheduler, CommandController& commandController,
	           EventDistributor& eventDistributor,
	           MSXEventDistributor& msxEventDistributor,
	           ReverseManager& reverseManager);
	virtual ~EventDelay();

	void sync(EmuTime::param time);
	void flush();

private:
	typedef std::shared_ptr<const Event> EventPtr;

	// EventListener
	virtual int signalEvent(const EventPtr& event);

	// Schedulable
	virtual void executeUntil(EmuTime::param time, int userData);

	EventDistributor& eventDistributor;
	MSXEventDistributor& msxEventDistributor;

	std::vector<EventPtr> toBeScheduledEvents;
	std::deque<EventPtr> scheduledEvents;

#if PLATFORM_ANDROID
	std::vector<std::pair<int, EventPtr>> nonMatchedKeyPresses;
#endif

	EmuTime prevEmu;
	uint64_t prevReal;
	const std::unique_ptr<FloatSetting> delaySetting;
};

} // namespace openmsx

#endif
