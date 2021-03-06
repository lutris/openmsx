#include "RS232Connector.hh"
#include "RS232Device.hh"
#include "DummyRS232Device.hh"
#include "checked_cast.hh"
#include "serialize.hh"
#include "memory.hh"

namespace openmsx {

RS232Connector::RS232Connector(PluggingController& pluggingController,
                               string_ref name)
	: Connector(pluggingController, name,
	            make_unique<DummyRS232Device>())
{
}

RS232Connector::~RS232Connector()
{
}

const std::string RS232Connector::getDescription() const
{
	return "Serial RS232 connector";
}

string_ref RS232Connector::getClass() const
{
	return "RS232";
}

RS232Device& RS232Connector::getPluggedRS232Dev() const
{
	return *checked_cast<RS232Device*>(&getPlugged());
}

template<typename Archive>
void RS232Connector::serialize(Archive& ar, unsigned /*version*/)
{
	ar.template serializeBase<Connector>(*this);
}
INSTANTIATE_SERIALIZE_METHODS(RS232Connector);

} // namespace openmsx
