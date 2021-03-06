#ifndef MIDIINDEVICE_HH
#define MIDIINDEVICE_HH

#include "Pluggable.hh"

namespace openmsx {

class MidiInDevice : public Pluggable
{
public:
	// Pluggable (part)
	virtual string_ref getClass() const;

	virtual void signal(EmuTime::param time) = 0;
};

} // namespace openmsx

#endif
