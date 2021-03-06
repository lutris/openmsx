// This implementation is based on the reverse-engineering effort done
// by 'SD-snatcher'. See here:
//   http://www.msx.org/news/en/msx-touchpad-protocol-reverse-engineered
//   http://www.msx.org/wiki/Touchpad
// Also thanks to Manuel Bilderbeek for donating a Philips NMS-1150 that
// made this effort possible.

#include "Touchpad.hh"
#include "MSXEventDistributor.hh"
#include "StateChangeDistributor.hh"
#include "InputEvents.hh"
#include "StateChange.hh"
#include "StringSetting.hh"
#include "CommandException.hh"
#include "Clock.hh"
#include "Math.hh"
#include "checked_cast.hh"
#include "serialize.hh"
#include "serialize_meta.hh"
#include <iostream>
#include <SDL.h>

using std::shared_ptr;

namespace openmsx {

class TouchpadState : public StateChange
{
public:
	TouchpadState() {} // for serialize
	TouchpadState(EmuTime::param time,
	              byte x_, byte y_, bool touch_, bool button_)
		: StateChange(time)
		, x(x_), y(y_), touch(touch_), button(button_) {}
	byte getX()      const { return x; }
	byte getY()      const { return y; }
	bool getTouch()  const { return touch; }
	bool getButton() const { return button; }

	template<typename Archive> void serialize(Archive& ar, unsigned /*version*/)
	{
		ar.template serializeBase<StateChange>(*this);
		ar.serialize("x",      x);
		ar.serialize("y",      y);
		ar.serialize("touch",  touch);
		ar.serialize("button", button);
	}
private:
	byte x, y;
	bool touch, button;
};
REGISTER_POLYMORPHIC_CLASS(StateChange, TouchpadState, "TouchpadState");


Touchpad::Touchpad(MSXEventDistributor& eventDistributor_,
                   StateChangeDistributor& stateChangeDistributor_,
                   CommandController& commandController)
	: eventDistributor(eventDistributor_)
	, stateChangeDistributor(stateChangeDistributor_)
	, start(EmuTime::zero)
	, hostX(0), hostY(0), hostButtons(0)
	, x(0), y(0), touch(false), button(false)
	, shift(0), channel(0), last(0)
{
	transformSetting = make_unique<StringSetting>(commandController,
		"touchpad_transform_matrix",
		"2x3 matrix to transform host mouse coordinates to "
		"MSX touchpad coordinates, see manual for details",
		"{ 256 0 0 } { 0 256 0 }");
	transformSetting->setChecker([this](TclObject& newValue) {
		try {
			parseTransformMatrix(newValue);
		} catch (CommandException& e) {
			throw CommandException(
				"Invalid transformation matrix: " + e.getMessage());
		}
	});
	try {
		parseTransformMatrix(transformSetting->getValue());
	} catch (CommandException& e) {
		// should only happen when settings.xml was manually edited
		std::cerr << e.getMessage() << std::endl;
		// fill in safe default values
		m[0][0] = 256.0; m[0][1] =   0.0; m[0][2] = 0.0;
		m[1][0] =   0.0; m[1][1] = 256.0; m[1][2] = 0.0;
	}
}

Touchpad::~Touchpad()
{
	if (isPluggedIn()) {
		Touchpad::unplugHelper(EmuTime::dummy());
	}
}

void Touchpad::parseTransformMatrix(const TclObject& value)
{
	if (value.getListLength() != 2) {
		throw CommandException("must have 2 rows");
	}
	for (int i = 0; i < 2; ++i) {
		TclObject row = value.getListIndex(i);
		if (row.getListLength() != 3) {
			throw CommandException("each row must have 3 elements");
		}
		for (int j = 0; j < 3; ++j) {
			m[i][j] = row.getListIndex(j).getDouble();
		}
	}
}

// Pluggable
const std::string& Touchpad::getName() const
{
	static const std::string name("touchpad");
	return name;
}

string_ref Touchpad::getDescription() const
{
	return "MSX Touchpad";
}

void Touchpad::plugHelper(Connector& /*connector*/, EmuTime::param /*time*/)
{
	eventDistributor.registerEventListener(*this);
	stateChangeDistributor.registerListener(*this);
}

void Touchpad::unplugHelper(EmuTime::param /*time*/)
{
	stateChangeDistributor.unregisterListener(*this);
	eventDistributor.unregisterEventListener(*this);
}

// JoystickDevice
byte Touchpad::read(EmuTime::param time)
{
	static const byte SENSE  = RD_PIN1;
	static const byte EOC    = RD_PIN2;
	static const byte SO     = RD_PIN3;
	static const byte BUTTON = RD_PIN4;

	byte result = SENSE | BUTTON; // 1-bit means not pressed
	if (touch)  result &= ~SENSE;
	if (button) result &= ~BUTTON;

	// EOC remains zero for 56 cycles after CS 0->1
	// TODO at what clock frequency does the UPD7001 run?
	//    400kHz is only the recommended value from the UPD7001 datasheet.
	static const EmuDuration delta = Clock<400000>::duration(56);
	if ((time - start) > delta) {
		result |= EOC;
	}

	if (shift & 0x80) result |= SO;

	return result;
}

void Touchpad::write(byte value, EmuTime::param time)
{
	static const byte SCK = WR_PIN6;
	static const byte SI  = WR_PIN7;
	static const byte CS  = WR_PIN8;

	byte diff = last ^ value;
	last = value;
	if (diff & CS) {
		if (value & CS) {
			// CS 0->1
			channel = shift & 3;
			start = time; // to keep EOC=0 for 56 cycles
		} else {
			// CS 1->0
			// Tested by SD-Snatcher (see RFE #252):
			//  When not touched X is always 0, and Y floats
			//  between 147 and 149 (mostly 148).
			shift = (channel == 0) ? (touch ? x :   0)
			      : (channel == 3) ? (touch ? y : 148)
			      : 0; // channel 1 and 2 return 0
		}
	}
	if (((value & (CS | SCK)) == SCK) && (diff & SCK)) {
		// SC=0 & SCK 0->1
		shift <<= 1;
		shift |= (value & SI) != 0;
	}
}

void Touchpad::transformCoords(int& x, int& y)
{
	if (SDL_Surface* surf = SDL_GetVideoSurface()) {
		double u = double(x) / surf->w;
		double v = double(y) / surf->h;
		x = m[0][0] * u + m[0][1] * v + m[0][2];
		y = m[1][0] * u + m[1][1] * v + m[1][2];
	}
	x = Math::clipIntToByte(x);
	y = Math::clipIntToByte(y);
}

// MSXEventListener
void Touchpad::signalEvent(const shared_ptr<const Event>& event,
                           EmuTime::param time)
{
	int x = hostX;
	int y = hostY;
	int b = hostButtons;
	switch (event->getType()) {
	case OPENMSX_MOUSE_MOTION_EVENT: {
		auto& mev = checked_cast<const MouseMotionEvent&>(*event);
		x = mev.getAbsX();
		y = mev.getAbsY();
		transformCoords(x, y);
		break;
	}
	case OPENMSX_MOUSE_BUTTON_DOWN_EVENT: {
		auto& butEv = checked_cast<const MouseButtonEvent&>(*event);
		switch (butEv.getButton()) {
		case MouseButtonEvent::LEFT:
			b |= 1;
			break;
		case MouseButtonEvent::RIGHT:
			b |= 2;
			break;
		default:
			// ignore other buttons
			break;
		}
		break;
	}
	case OPENMSX_MOUSE_BUTTON_UP_EVENT: {
		auto& butEv = checked_cast<const MouseButtonEvent&>(*event);
		switch (butEv.getButton()) {
		case MouseButtonEvent::LEFT:
			b &= ~1;
			break;
		case MouseButtonEvent::RIGHT:
			b &= ~2;
			break;
		default:
			// ignore other buttons
			break;
		}
		break;
	}
	default:
		// ignore
		break;
	}
	if ((x != hostX) || (y != hostY) || (b != hostButtons)) {
		hostX       = x;
		hostY       = y;
		hostButtons = b;
		createTouchpadStateChange(
			time, hostX, hostY,
			(hostButtons & 1) != 0,
			(hostButtons & 2) != 0);
	}
}

void Touchpad::createTouchpadStateChange(
	EmuTime::param time, byte x, byte y, bool touch, bool button)
{
	stateChangeDistributor.distributeNew(std::make_shared<TouchpadState>(
		time, x, y, touch, button));
}

// StateChangeListener
void Touchpad::signalStateChange(const shared_ptr<StateChange>& event)
{
	if (auto* ts = dynamic_cast<TouchpadState*>(event.get())) {
		x      = ts->getX();
		y      = ts->getY();
		touch  = ts->getTouch();
		button = ts->getButton();
	}
}

void Touchpad::stopReplay(EmuTime::param time)
{
	// TODO Get actual mouse state. Is it worth the trouble?
	if (x || y || touch || button) {
		stateChangeDistributor.distributeNew(
			std::make_shared<TouchpadState>(
				time, 0, 0, false, false));
	}
}


template<typename Archive>
void Touchpad::serialize(Archive& ar, unsigned /*version*/)
{
	// no need to serialize hostX, hostY, hostButtons,
	//                      transformSetting, m[][]
	ar.serialize("start", start);
	ar.serialize("x", x);
	ar.serialize("y", y);
	ar.serialize("touch", touch);
	ar.serialize("button", button);
	ar.serialize("shift", shift);
	ar.serialize("channel", channel);
	ar.serialize("last", last);

	if (ar.isLoader() && isPluggedIn()) {
		plugHelper(*getConnector(), EmuTime::dummy());
	}
}
INSTANTIATE_SERIALIZE_METHODS(Touchpad);
REGISTER_POLYMORPHIC_INITIALIZER(Pluggable, Touchpad, "Touchpad");

} // namespace openmsx
