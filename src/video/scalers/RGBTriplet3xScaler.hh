#ifndef RGBTRIPLET3XSCALER_HH
#define RGBTRIPLET3XSCALER_HH

#include "Scaler3.hh"
#include "PixelOperations.hh"
#include "Scanline.hh"

namespace openmsx {

class RenderSettings;
template<typename Pixel> class PolyLineScaler;

/** TODO
  */
template <class Pixel>
class RGBTriplet3xScaler : public Scaler3<Pixel>
{
public:
	RGBTriplet3xScaler(const PixelOperations<Pixel>& pixelOps,
	                   const RenderSettings& renderSettings);

protected:
	virtual void scaleImage(FrameSource& src, const RawFrame* superImpose,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);

	virtual void scaleBlank1to3(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scaleBlank2to3(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale2x1to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale2x2to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale1x1to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale1x2to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale4x1to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale4x2to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale2x1to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale2x2to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale8x1to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale8x2to9x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale4x1to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale4x2to3x3(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY);

private:
	void calcBlur(unsigned& c1, unsigned& c2);

	/**
	 * Calculates the RGB triplets.
	 * @param in Buffer of input pixels
	 * @param out Buffer of output pixels, should be 3x as long as input
	 * @param inwidth Width of the input buffer (in pixels)
	 */
	void rgbify(const Pixel* in, Pixel* out, unsigned inwidth, unsigned c1, unsigned c2);

	void scaleLine(const Pixel* srcLine, Pixel* dstLine,
	               PolyLineScaler<Pixel>& scale, unsigned tmpWidth,
	               unsigned c1, unsigned c2);
	void doScale1(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY,
		PolyLineScaler<Pixel>& scale);
	void doScale2(FrameSource& src,
		unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY,
		PolyLineScaler<Pixel>& scale);

	PixelOperations<Pixel> pixelOps;
	Scanline<Pixel> scanline;
	const RenderSettings& settings;
};

} // namespace openmsx

#endif
