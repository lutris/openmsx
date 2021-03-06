#ifndef SDLSURFACEPTR
#define SDLSURFACEPTR

#include <SDL.h>
#include <algorithm>
#include <new>
#include <cassert>
#include <cstdlib>

/** Wrapper around a SDL_Surface.
 *
 * Makes sure SDL_FreeSurface() is called when this object goes out of scope.
 * It's modeled after std::unique_ptr, so it has the usual get(), reset() and
 * release() methods. Like unique_ptr it can be moved but not copied.
 *
 * In addition to the SDL_Surface pointer, this wrapper also (optionally)
 * manages an extra memory buffer. Normally SDL_CreateRGBSurface() will
 * allocate/free an internal memory buffer for the surface. On construction
 * of the surface this buffer will be zero-initialized. Though in many cases
 * the surface will immediately be overwritten (so zero-initialization is
 * only extra overhead). It's possible to avoid this by creating the surface
 * using SDL_CreateRGBSurfaceFrom(). Though the downside of this is that you
 * have to manage the lifetime of the memory buffer yourself. And that's
 * exactly what this wrapper can do.
 *
 * As a bonus this wrapper has a getLinePtr() method to hide some of the
 * casting. But apart from this it doesn't try to abstract any SDL
 * functionality.
 */
class SDLSurfacePtr
{
public:
	/** Create a (software) surface with uninitialized pixel content.
	  * throws: bad_alloc (no need to check for nullptr). */
	SDLSurfacePtr(unsigned width, unsigned height, unsigned depth,
	              Uint32 rMask, Uint32 gMask, Uint32 bMask, Uint32 aMask)
	{
		assert((depth % 8) == 0);
		unsigned pitch = width * (depth >> 3);
		unsigned size = height * pitch;
		buffer = malloc(size);
		if (!buffer) throw std::bad_alloc();
		surface = SDL_CreateRGBSurfaceFrom(
			buffer, width, height, depth, pitch,
			rMask, gMask, bMask, aMask);
		if (!surface) {
			free(buffer);
			throw std::bad_alloc();
		}
	}

	// don't allow copy and assign
	// msvc doesn't yet support deleted functions.
	SDLSurfacePtr(const SDLSurfacePtr&) /*= delete*/;
	SDLSurfacePtr& operator=(const SDLSurfacePtr&) /*= delete*/;

	explicit SDLSurfacePtr(SDL_Surface* surface_ = nullptr,
	                       void* buffer_ = nullptr)
		: surface(surface_)
		, buffer(buffer_)
	{
	}

	SDLSurfacePtr(SDLSurfacePtr&& other)
		: surface(other.surface)
		, buffer(other.buffer)
	{
		other.surface = nullptr;
		other.buffer = nullptr;
	}

	~SDLSurfacePtr()
	{
		if (surface) {
			SDL_FreeSurface(surface);
		}
		free(buffer);
	}

	void reset(SDL_Surface* surface_ = nullptr)
	{
		SDLSurfacePtr temp(surface_);
		temp.swap(*this);
	}

	SDL_Surface* get()
	{
		return surface;
	}
	const SDL_Surface* get() const
	{
		return surface;
	}

	void swap(SDLSurfacePtr& other)
	{
		std::swap(surface, other.surface);
		std::swap(buffer,  other.buffer );
	}

	SDLSurfacePtr& operator=(SDLSurfacePtr&& other)
	{
		SDLSurfacePtr tmp(other.surface, other.buffer);
		other.surface = nullptr;
		other.buffer = nullptr;
		tmp.swap(*this);
		return *this;
	}

	SDL_Surface& operator*()
	{
		return *surface;
	}
	const SDL_Surface& operator*() const
	{
		return *surface;
	}

	SDL_Surface* operator->()
	{
		return surface;
	}
	const SDL_Surface* operator->() const
	{
		return surface;
	}

	explicit operator bool() const
	{
		return get() != nullptr;
	}

	void* getLinePtr(unsigned y)
	{
		assert(y < unsigned(surface->h));
		return static_cast<Uint8*>(surface->pixels) + y * surface->pitch;
	}
	const void* getLinePtr(unsigned y) const
	{
		return const_cast<SDLSurfacePtr*>(this)->getLinePtr(y);
	}

private:
	SDL_Surface* surface;
	void* buffer;
};

#endif
