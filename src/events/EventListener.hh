#ifndef EVENTLISTENER_HH
#define EVENTLISTENER_HH

#include <memory>

namespace openmsx {

class Event;

class EventListener
{
public:
	virtual ~EventListener() {}

	/**
	 * This method gets called when an event you are subscribed to occurs.
	 * @result Must return a bitmask of EventListener priorities. When a
	 *         bit is set, this event won't be delivered to listeners with
	 *         that priority. It's only allowed/possible to block an event
	 *         for listeners with a strictly lower priority than this
	 *         listener. Returning 0 means don't block the event for any
	 *         listeners.
	 */
	virtual int signalEvent(const std::shared_ptr<const Event>& event) = 0;

protected:
	EventListener() {}
};

} // namespace openmsx

#endif // EVENTLISTENER_HH
