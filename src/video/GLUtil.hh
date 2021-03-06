#ifndef GLUTIL_HH
#define GLUTIL_HH

// Check for availability of OpenGL.
#include "components.hh"
#if COMPONENT_GL

// Include GLEW headers.
#include <GL/glew.h>
// Include OpenGL headers.
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "MemBuffer.hh"
#include "noncopyable.hh"
#include "build-info.hh"
#include <string>
#include <cassert>

namespace openmsx {

namespace GLUtil {

// TODO this needs glu, but atm we don't link against glu (in windows)
//void checkGLError(const std::string& prefix);

} // namespace GLUtil


/** Most basic/generic texture: only contains a texture ID.
  * Current implementation always assumes 2D textures.
  */
class Texture
{
public:
	/** Default constructor, allocate a openGL texture name. */
	Texture();

	/** Move constructor and assignment. */
	Texture(Texture&& other)
		: textureId(other.textureId)
	{
		other.textureId = 0; // 0 is not a valid openGL texture name
	}
	Texture& operator=(Texture&& other) {
		std::swap(textureId, other.textureId);
		return *this;
	}

	/** Release openGL texture name. */
	~Texture();

	/** Makes this texture the active GL texture.
	  * The other methods of this class and its subclasses will implicitly
	  * bind the texture, so you only need this method to explicitly bind
	  * this texture for use in GL function calls outside of this class.
	  */
	void bind() {
		glBindTexture(GL_TEXTURE_2D, textureId);
	}

	/** Enables bilinear interpolation for this texture.
	  */
	void enableInterpolation();

	/** Disables bilinear interpolation for this texture and uses nearest
	  * neighbour instead.
	  */
	void disableInterpolation();

	void setWrapMode(bool wrap);

	/** Draws this texture as a rectangle on the frame buffer.
	  */
	void drawRect(GLfloat tx, GLfloat ty, GLfloat twidth, GLfloat theight,
	              GLint   x,  GLint   y,  GLint   width,  GLint   height);

protected:
	GLuint textureId;

private:
	// Disable copy, assign.
	Texture(const Texture&);
	Texture& operator=(const Texture&);

	friend class FrameBufferObject;
};

class ColorTexture : public Texture
{
public:
	/** Default constructor, zero-sized texture. */
	ColorTexture() : width(0), height(0) {}

	/** Move constructor and assignment. */
	ColorTexture(ColorTexture&& other)
		: Texture(std::move(other))
	{
		width  = other.width;
		height = other.height;
	}
	ColorTexture& operator=(ColorTexture&& other) {
		*this = std::move(other);
		width  = other.width;
		height = other.height;
		return *this;
	}

	/** Create color texture with given size.
	  * Initial content is undefined.
	  */
	ColorTexture(GLsizei width, GLsizei height);
	void resize(GLsizei width, GLsizei height);

	GLsizei getWidth () const { return width;  }
	GLsizei getHeight() const { return height; }

private:
	// Disable copy, assign.
	ColorTexture(const ColorTexture&);
	ColorTexture& operator=(const ColorTexture&);

	GLsizei width;
	GLsizei height;
};

class LuminanceTexture : public Texture
{
public:
	/** Move constructor and assignment. */
	LuminanceTexture(LuminanceTexture&& other)
		: Texture(std::move(other))
	{
	}
	LuminanceTexture& operator=(LuminanceTexture&& other) {
		*this = std::move(other);
		return *this;
	}

	/** Create grayscale texture with given size.
	  * Initial content is undefined.
	  */
	LuminanceTexture(GLsizei width, GLsizei height);

	/** Redefines (part of) the image for this texture.
	  */
	void updateImage(GLint x, GLint y,
	                 GLsizei width, GLsizei height,
	                 GLbyte* data);

private:
	// Disable copy, assign.
	LuminanceTexture(const LuminanceTexture&);
	LuminanceTexture& operator=(const LuminanceTexture&);
};

class FrameBufferObject //: public noncopyable
{
public:
	FrameBufferObject();
	explicit FrameBufferObject(Texture& texture);
	FrameBufferObject(FrameBufferObject&& other)
		: bufferId(other.bufferId)
	{
		other.bufferId = 0;
	}
	FrameBufferObject& operator=(FrameBufferObject&& other) {
		std::swap(bufferId, other.bufferId);
		return *this;
	}
	~FrameBufferObject();

	void push();
	void pop();

private:
	GLuint bufferId;
};

struct PixelBuffers
{
	/** Global switch to disable pixel buffers using the "-nopbo" option.
	  */
	static bool enabled;
};

/** Wrapper around a pixel buffer.
  * The pixel buffer will be allocated in VRAM if possible, in main RAM
  * otherwise.
  * The pixel type is templatized T.
  */
template <typename T> class PixelBuffer //: public noncopyable
{
public:
	PixelBuffer();
	PixelBuffer(PixelBuffer&& other);
	PixelBuffer& operator=(PixelBuffer&& other);
	~PixelBuffer();

	/** Are PBOs supported by this openGL implementation?
	  * This class implements a SW fallback in case PBOs are not directly
	  * supported by this openGL implementation, but it will probably
	  * be a lot slower.
	  */
	bool openGLSupported() const;

	/** Sets the image for this buffer.
	  * TODO: Actually, only image size for now;
	  *       later, if we need it, image data too.
	  */
	void setImage(GLuint width, GLuint height);

	/** Bind this PixelBuffer. Must be called before the methods
	  * getOffset() or mapWrite() are used. (Is a wrapper around
	  * glBindBuffer).
	  */
	void bind() const;

	/** Unbind this buffer.
	  */
	void unbind() const;

	/** Gets a pointer relative to the start of this buffer.
	  * You must not dereference this pointer, but you can pass it to
	  * glTexImage etc when this buffer is bound as the source.
	  * @pre This PixelBuffer must be bound (see bind()) before calling
	  *      this method.
	  */
	T* getOffset(GLuint x, GLuint y);

	/** Maps the contents of this buffer into memory. The returned buffer
	  * is write-only (reading could be very slow or even result in a
	  * segfault).
	  * @return Pointer through which you can write pixels to this buffer,
	  *         or 0 if the buffer could not be mapped.
	  * @pre This PixelBuffer must be bound (see bind()) before calling
	  *      this method.
	  */
	T* mapWrite();

	/** Unmaps the contents of this buffer.
	  * After this call, you must no longer use the pointer returned by
	  * mapWrite.
	  */
	void unmap() const;

private:
	/** Buffer for main RAM fallback (not allocated in the normal case).
	  */
	MemBuffer<T> allocated;

	/** Handle of the GL buffer, or 0 if no GL buffer is available.
	  */
	GLuint bufferId;

	/** Number of pixels per line.
	  */
	GLuint width;

	/** Number of lines.
	  */
	GLuint height;
};

// class PixelBuffer

template <typename T>
PixelBuffer<T>::PixelBuffer()
{
	if (PixelBuffers::enabled && GLEW_ARB_pixel_buffer_object) {
		glGenBuffers(1, &bufferId);
	} else {
		//std::cerr << "OpenGL pixel buffers are not available" << std::endl;
		bufferId = 0;
	}
}

template <typename T>
PixelBuffer<T>::PixelBuffer(PixelBuffer<T>&& other)
	: allocated(std::move(other.allocated))
	, bufferId(other.bufferId)
	, width(other.width)
	, height(other.height)
{
	other.bufferId = 0;
}

template <typename T>
PixelBuffer<T>& PixelBuffer<T>::operator=(PixelBuffer<T>&& other)
{
	std::swap(allocated, other.allocated);
	std::swap(bufferId,  other.bufferId);
	std::swap(width,     other.width);
	std::swap(height,    other.height);
	return *this;
}

template <typename T>
PixelBuffer<T>::~PixelBuffer()
{
	glDeleteBuffers(1, &bufferId); // ok to delete '0'
}

template <typename T>
bool PixelBuffer<T>::openGLSupported() const
{
	return bufferId != 0;
}

template <typename T>
void PixelBuffer<T>::setImage(GLuint width, GLuint height)
{
	this->width = width;
	this->height = height;
	if (bufferId != 0) {
		bind();
		// TODO make performance hint configurable?
		glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB,
		             width * height * 4,
		             nullptr, // leave data undefined
		             GL_STREAM_DRAW); // performance hint
		unbind();
	} else {
		allocated.resize(width * height);
	}
}

template <typename T>
void PixelBuffer<T>::bind() const
{
	if (bufferId != 0) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bufferId);
	}
}

template <typename T>
void PixelBuffer<T>::unbind() const
{
	if (bufferId != 0) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}
}

template <typename T>
T* PixelBuffer<T>::getOffset(GLuint x, GLuint y)
{
	assert(x < width);
	assert(y < height);
	unsigned offset = x + width * y;
	if (bufferId != 0) {
		return static_cast<T*>(nullptr) + offset;
	} else {
		return &allocated[offset];
	}
}

template <typename T>
T* PixelBuffer<T>::mapWrite()
{
	if (bufferId != 0) {
		return reinterpret_cast<T*>(glMapBuffer(
			GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY));
	} else {
		return allocated.data();
	}
}

template <typename T>
void PixelBuffer<T>::unmap() const
{
	if (bufferId != 0) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
	}
}



/** Wrapper around an OpenGL shader: a program executed on the GPU.
  * This class is a base class for vertex and fragment shaders.
  */
class Shader
{
public:
	/** Returns true iff this shader is loaded and compiled without errors.
	  */
	bool isOK() const;

protected:
	/** Instantiates a shader.
	  * @param type The shader type: GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
	  * @param filename The GLSL source code for the shader.
	  */
	Shader(GLenum type, const std::string& filename);
	Shader(GLenum type, const std::string& header,
	                    const std::string& filename);
	~Shader();

private:
	void init(GLenum type, const std::string& header,
	                       const std::string& filename);

	friend class ShaderProgram;

	GLuint handle;
};

/** Wrapper around an OpenGL vertex shader:
  * a program executed on the GPU that computes per-vertex stuff.
  */
class VertexShader : public Shader
{
public:
	/** Instantiates a vertex shader.
	  * @param filename The GLSL source code for the shader.
	  */
	explicit VertexShader(const std::string& filename);
	VertexShader(const std::string& header, const std::string& filename);
};

/** Wrapper around an OpenGL fragment shader:
  * a program executed on the GPU that computes the colors of pixels.
  */
class FragmentShader : public Shader
{
public:
	/** Instantiates a fragment shader.
	  * @param filename The GLSL source code for the shader.
	  */
	explicit FragmentShader(const std::string& filename);
	FragmentShader(const std::string& header, const std::string& filename);
};

/** Wrapper around an OpenGL program:
  * a collection of vertex and fragment shaders.
  */
class ShaderProgram : public noncopyable
{
public:
	ShaderProgram();
	~ShaderProgram();

	/** Returns true iff this program was linked without errors.
	  * Note that this will certainly return false until link() is called.
	  */
	bool isOK() const;

	/** Adds a given shader to this program.
	  */
	void attach(const Shader& shader);

	/** Links all attached shaders together into one program.
	  * This should be done before activating the program.
	  */
	void link();

	/** Gets a reference to a uniform variable declared in the shader source.
	  * Note that you have to activate this program before you can change
	  * the uniform variable's value.
	  */
	GLint getUniformLocation(const char* name) const;

	/** Makes this program the active shader program.
	  * This requires that the program is already linked.
	  */
	void activate() const;

	/** Deactivates all shader programs.
	  */
	static void deactivate();

	void validate();

private:
	GLuint handle;
};

} // namespace openmsx

#endif // COMPONENT_GL
#endif // GLUTIL_HH
