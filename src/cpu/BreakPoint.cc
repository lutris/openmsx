#include "BreakPoint.hh"
#include "TclObject.hh"

namespace openmsx {

unsigned BreakPoint::lastId = 0;

BreakPoint::BreakPoint(GlobalCliComm& cliComm, word address_,
                       TclObject command, TclObject condition)
	: BreakPointBase(cliComm, command, condition)
	, id(++lastId)
	, address(address_)
{
}

word BreakPoint::getAddress() const
{
	return address;
}

unsigned BreakPoint::getId() const
{
	return id;
}

} // namespace openmsx
