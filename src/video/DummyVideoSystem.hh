#ifndef DUMMYVIDEOSYSTEM_HH
#define DUMMYVIDEOSYSTEM_HH

#include "VideoSystem.hh"
#include "components.hh"

namespace openmsx {

class DummyVideoSystem : public VideoSystem
{
public:
	// VideoSystem interface:
	virtual std::unique_ptr<Rasterizer> createRasterizer(VDP& vdp);
	virtual std::unique_ptr<V9990Rasterizer> createV9990Rasterizer(
		V9990& vdp);
#if COMPONENT_LASERDISC
	virtual std::unique_ptr<LDRasterizer> createLDRasterizer(
		LaserdiscPlayer& ld);
#endif
	virtual void flush();
	virtual OutputSurface* getOutputSurface();
};

} // namespace openmsx

#endif
