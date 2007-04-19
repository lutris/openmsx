// $Id$

#ifndef WAVWRITER_HH
#define WAVWRITER_HH

#include "noncopyable.hh"
#include <string>
#include <cstdio>

namespace openmsx {

class WavWriter : private noncopyable
{
public:
	WavWriter(const std::string& filename,
	          unsigned channels, unsigned bits, unsigned frequency);
	~WavWriter();

	void write8mono(unsigned char val);
	void write8mono(unsigned char* buf, unsigned len);
	void write16stereo(short* buffer, unsigned samples);
	void write16mono  (int* buffer, unsigned samples);
	void write16stereo(int* buffer, unsigned samples);

	/** Flush data to file and update header. Try to make (possibly)
	  * incomplete file already usable for external programs.
	  */
	void flush();

private:
	FILE* wavfp;
	unsigned bytes;
};

} // namespace openmsx

#endif
