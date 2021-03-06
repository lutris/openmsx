#include "RawFrame.hh"
#include "MemoryOps.hh"
#include <cstdint>
#include <SDL.h>

namespace openmsx {

RawFrame::RawFrame(
		const SDL_PixelFormat& format, unsigned maxWidth_, unsigned height)
	: FrameSource(format)
	, lineWidths(height)
	, maxWidth(maxWidth_)
{
	setHeight(height);
	unsigned bytesPerPixel = format.BytesPerPixel;

	// Allocate memory, make sure each line starts at a 64 byte boundary:
	// - SSE instructions need 16 byte aligned data
	// - cache line size on many CPUs is 64 bytes
	pitch = ((bytesPerPixel * maxWidth) + 63) & ~63;
	data = reinterpret_cast<char*>(
		MemoryOps::mallocAligned(64, pitch * height));

	maxWidth = pitch / bytesPerPixel; // adjust maxWidth

	// Start with a black frame.
	init(FIELD_NONINTERLACED);
	for (unsigned line = 0; line < height; line++) {
		if (bytesPerPixel == 2) {
			setBlank(line, static_cast<uint16_t>(0));
		} else {
			setBlank(line, static_cast<uint32_t>(0));
		}
	}
}

RawFrame::~RawFrame()
{
	MemoryOps::freeAligned(data);
}

unsigned RawFrame::getLineWidth(unsigned line) const
{
	assert(line < getHeight());
	return lineWidths[line];
}

const void* RawFrame::getLineInfo(
	unsigned line, unsigned& width,
	void* /*buf*/, unsigned /*bufWidth*/) const
{
	assert(line < getHeight());
	width = lineWidths[line];
	return data + line * pitch;
}

unsigned RawFrame::getRowLength() const
{
	return maxWidth; // in pixels (not in bytes)
}

bool RawFrame::hasContiguousStorage() const
{
	return true;
}

} // namespace openmsx
