#ifndef WIN32_ARG_GEN_HH
#define WIN32_ARG_GEN_HH

#ifdef _WIN32

#include "MemBuffer.hh"

namespace openmsx {

class ArgumentGenerator
{
public:
	~ArgumentGenerator();
	char** GetArguments(int& argc);

private:
	MemBuffer<char*> argv;
};

#endif

} // namespace openmsx

#endif // WIN32_ARG_GEN_HH
