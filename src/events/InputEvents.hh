#ifndef INPUTEVENTS_HH
#define INPUTEVENTS_HH

#include "Event.hh"
#include "Keys.hh"
#include <cstdint>
#include <memory>

namespace openmsx {

class TimedEvent : public Event
{
public:
	/** Query creation time. */
	uint64_t getRealTime() const { return realtime; }
protected:
	explicit TimedEvent(EventType type);
private:
	const uint64_t realtime;
};


class KeyEvent : public TimedEvent
{
public:
	Keys::KeyCode getKeyCode() const { return keyCode; }
	uint16_t getUnicode() const { return unicode; }

protected:
	KeyEvent(EventType type, Keys::KeyCode keyCode, uint16_t unicode);

private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
	const Keys::KeyCode keyCode;
	const uint16_t unicode;
};

class KeyUpEvent : public KeyEvent
{
public:
	explicit KeyUpEvent(Keys::KeyCode keyCode);
	KeyUpEvent(Keys::KeyCode keyCode, uint16_t unicode);
};

class KeyDownEvent : public KeyEvent
{
public:
	explicit KeyDownEvent(Keys::KeyCode keyCode);
	KeyDownEvent(Keys::KeyCode keyCode, uint16_t unicode);
};


class MouseButtonEvent : public TimedEvent
{
public:
	static const unsigned LEFT      = 1;
	static const unsigned MIDDLE    = 2;
	static const unsigned RIGHT     = 3;
	static const unsigned WHEELUP   = 4;
	static const unsigned WHEELDOWN = 5;

	unsigned getButton() const { return button; }

protected:
	MouseButtonEvent(EventType type, unsigned button_);
	void toStringHelper(TclObject& result) const;

private:
	virtual bool lessImpl(const Event& other) const;
	const unsigned button;
};

class MouseButtonUpEvent : public MouseButtonEvent
{
public:
	explicit MouseButtonUpEvent(unsigned button);
private:
	virtual void toStringImpl(TclObject& result) const;
};

class MouseButtonDownEvent : public MouseButtonEvent
{
public:
	explicit MouseButtonDownEvent(unsigned button);
private:
	virtual void toStringImpl(TclObject& result) const;
};

class MouseMotionEvent : public TimedEvent
{
public:
	MouseMotionEvent(int xrel, int yrel, int xabs, int yabs);
	int getX() const    { return xrel; }
	int getY() const    { return yrel; }
	int getAbsX() const { return xabs; }
	int getAbsY() const { return yabs; }
private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
	const int xrel;
	const int yrel;
	const int xabs;
	const int yabs;
};

class MouseMotionGroupEvent : public Event
{
public:
	MouseMotionGroupEvent();

private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
	virtual bool matches(const Event& other) const;
};


class JoystickEvent : public TimedEvent
{
public:
	unsigned getJoystick() const { return joystick; }

protected:
	JoystickEvent(EventType type, unsigned joystick);
	void toStringHelper(TclObject& result) const;

private:
	virtual bool lessImpl(const Event& other) const;
	virtual bool lessImpl(const JoystickEvent& other) const = 0;
	const unsigned joystick;
};

class JoystickButtonEvent : public JoystickEvent
{
public:
	unsigned getButton() const { return button; }

protected:
	JoystickButtonEvent(EventType type, unsigned joystick, unsigned button);
	void toStringHelper(TclObject& result) const;

private:
	virtual bool lessImpl(const JoystickEvent& other) const;
	const unsigned button;
};

class JoystickButtonUpEvent : public JoystickButtonEvent
{
public:
	JoystickButtonUpEvent(unsigned joystick, unsigned button);
private:
	virtual void toStringImpl(TclObject& result) const;
};

class JoystickButtonDownEvent : public JoystickButtonEvent
{
public:
	JoystickButtonDownEvent(unsigned joystick, unsigned button);
private:
	virtual void toStringImpl(TclObject& result) const;
};

class JoystickAxisMotionEvent : public JoystickEvent
{
public:
	static const unsigned X_AXIS = 0;
	static const unsigned Y_AXIS = 1;

	JoystickAxisMotionEvent(unsigned joystick, unsigned axis, short value);
	unsigned getAxis() const { return axis; }
	short getValue() const { return value; }

private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const JoystickEvent& other) const;
	const unsigned axis;
	const short value;
};


class FocusEvent : public Event
{
public:
	explicit FocusEvent(bool gain);

	bool getGain() const { return gain; }

private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
	const bool gain;
};


class ResizeEvent : public Event
{
public:
	ResizeEvent(unsigned x, unsigned y);

	unsigned getX() const { return x; }
	unsigned getY() const { return y; }

private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
	const unsigned x;
	const unsigned y;
};


class QuitEvent : public Event
{
public:
	QuitEvent();
private:
	virtual void toStringImpl(TclObject& result) const;
	virtual bool lessImpl(const Event& other) const;
};

/** OSD events are triggered by other events. They aggregate keyboard and
 * joystick events into one set of events that can be used to e.g. control
 * OSD elements.
 */
class OsdControlEvent : public TimedEvent
{
public:
	enum { LEFT_BUTTON, RIGHT_BUTTON, UP_BUTTON, DOWN_BUTTON,
		A_BUTTON, B_BUTTON };

	unsigned getButton() const { return button; }

	/** Get the event that actually triggered the creation of this event.
	 * Typically this will be a keyboard or joystick event. This could
	 * also return nullptr (after a toString/fromString conversion).
	 * For the current use (key-repeat) this is ok. */
	/** Normally all events should stop the repeat process in 'bind -repeat',
	 * but in case of OsdControlEvent there are two exceptions:
	 *  - we should not stop because of the original host event that
	 *    actually generated this 'artificial' OsdControlEvent.
	 *  - if the original host event is a joystick motion event, we
	 *    should not stop repeat for 'small' relative new joystick events.
	 */
	virtual bool isRepeatStopper(const Event& other) const;

protected:
	OsdControlEvent(EventType type, unsigned button_,
	                const std::shared_ptr<const Event>& origEvent);
	void toStringHelper(TclObject& result) const;

private:
	virtual bool lessImpl(const Event& other) const;
	const std::shared_ptr<const Event> origEvent;
	const unsigned button;
};

class OsdControlReleaseEvent : public OsdControlEvent
{
public:
	OsdControlReleaseEvent(unsigned button,
	                       const std::shared_ptr<const Event>& origEvent);
private:
	virtual void toStringImpl(TclObject& result) const;
};

class OsdControlPressEvent : public OsdControlEvent
{
public:
	OsdControlPressEvent(unsigned button,
	                     const std::shared_ptr<const Event>& origEvent);
private:
	virtual void toStringImpl(TclObject& result) const;
};

} // namespace openmsx

#endif
