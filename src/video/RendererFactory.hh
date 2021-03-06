#ifndef RENDERERFACTORY_HH
#define RENDERERFACTORY_HH

#include <memory>
#include "components.hh"

namespace openmsx {

class Reactor;
class CommandController;
class Display;
class VideoSystem;
class Renderer;
class VDP;
class V9990Renderer;
class V9990;
class LDRenderer;
class LaserdiscPlayer;
template <typename T> class EnumSetting;

/** Interface for renderer factories.
  * Every Renderer type has its own RendererFactory.
  * A RendererFactory can be queried about the availability of the
  * associated Renderer and can instantiate that Renderer.
  */
namespace RendererFactory
{
	/** Enumeration of Renderers known to openMSX.
	  * This is the full list, the list of available renderers may be smaller.
	  */
	enum RendererID { UNINITIALIZED, DUMMY, SDL,
	                  SDLGL_PP, SDLGL_FB16, SDLGL_FB32 };

	typedef EnumSetting<RendererID> RendererSetting;

	/** Create the video system required by the current renderer setting.
	  */
	std::unique_ptr<VideoSystem> createVideoSystem(Reactor& reactor);

	/** Create the Renderer selected by the current renderer setting.
	  * @param vdp The VDP whose display will be rendered.
	  * @param display TODO
	  */
	std::unique_ptr<Renderer> createRenderer(VDP& vdp, Display& display);

	/** Create the V9990 Renderer selected by the current renderer setting.
	  * @param vdp The V9990 VDP whose display will be rendered.
	  * @param display TODO
	  */
	std::unique_ptr<V9990Renderer> createV9990Renderer(
		V9990& vdp, Display& display);

#if COMPONENT_LASERDISC
	/** Create the Laserdisc Renderer
	  * @param ld The Laserdisc player whose display will be rendered.
	  * @param display TODO
	  */
	std::unique_ptr<LDRenderer> createLDRenderer(
		LaserdiscPlayer& ld, Display& display);
#endif

	/** Create the renderer setting.
	  * The map of this setting contains only the available renderers.
	  */
	std::unique_ptr<RendererSetting> createRendererSetting(
		CommandController& commandController);

} // namespace RendererFactory
} // namespace openmsx

#endif
