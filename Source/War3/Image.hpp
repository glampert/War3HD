
// ================================================================================================
// -*- C++ -*-
// File: Image.hpp
// Author: Guilherme R. Lampert
// Created on: 25/11/15
// Brief: Texture/Image helper classes.
// ================================================================================================

#ifndef WAR3_IMAGE_HPP
#define WAR3_IMAGE_HPP

#include "War3/Common.hpp"
#include <vector>

namespace War3
{

// ========================================================
// class Image:
// ========================================================

class Image final
{
public:

    using PixelBuffer = std::vector<UByte>;

    enum class TargetUsage
    {
        Null = 0,
        Texture2D
    };

    enum class PixelFormat
    {
        Null      = 0,
        Grayscale = 1, // Depth buffer textures
        RGBA_8888 = 4  // All textures used by the game
    };

    enum class Filter
    {
        Default,    // Current default filter defined by the renderer.
        Nearest,    // Nearest neighbor (or Manhattan Distance) filtering. Worst quality, best performance.
        Bilinear,   // Cheap bi-linear filtering. Low quality but good performance.
        Trilinear,  // Intermediate tri-linear filtering. Reasonable quality, average performance.
        Anisotropic // Anisotropic filtering. Best quality, most expensive. Paired with an anisotropy amount.
    };

    Image(PixelBuffer && pb, PixelFormat fmt, TargetUsage usage,
          int idx, int w, int h, int lvl) noexcept;

    // Not copyable.
    Image(const Image &) = delete;
    Image & operator = (const Image &) = delete;

    // Movable.
    Image(Image && other) noexcept;
    Image & operator = (Image && other) noexcept;

    // Saving to file (path must already exist!):
    bool savePng(const std::string & filename) const noexcept;
    bool saveTga(const std::string & filename) const noexcept;

    //
    // Misc accessors:
    //

    const PixelBuffer & getPixels() const noexcept { return pixels; }

    PixelFormat getFormat() const noexcept { return format; }
    TargetUsage getTarget() const noexcept { return target; }

    int getIndex()  const noexcept { return index;  }
    int getWidth()  const noexcept { return width;  }
    int getHeight() const noexcept { return height; }
    int getLevel()  const noexcept { return level;  }

    bool isValid() const noexcept
    {
        return (width > 0 && height > 0 &&
                format != PixelFormat::Null &&
                !pixels.empty());
    }

private:

    void invalidate() noexcept;

    PixelBuffer pixels;
    PixelFormat format;
    TargetUsage target;

    int index;
    int width;
    int height;
    int level;
};

// ========================================================
// OpenGL texture helpers:
// ========================================================

void bindGLTexture(UInt target, UInt texHandle, int tmu = -1) noexcept;
void setGLTextureFiltering(UInt target, Image::Filter filter, bool withMipmaps) noexcept;
void setGLPixelAlignment(UInt packAlign, UInt width, UInt bytesPerPix) noexcept;

Image::TargetUsage targetUsageFromGLEnum(UInt target) noexcept;
Image::PixelFormat pixelFormatFromGLEnum(UInt internal, UInt format, UInt type) noexcept;

// ========================================================
// class ImageManager:
// ========================================================

class ImageManager final
{
public:

    ImageManager() = default;
    ~ImageManager();

    // Not copyable.
    ImageManager(const ImageManager &) = delete;
    ImageManager & operator = (const ImageManager &) = delete;

    void saveAllImagesToFile() const;

    //
    // Intercept OpenGL texture functions:
    //

    void genTextures(int n, UInt * indexes);
    void deleteTextures(int n, const UInt * indexes);
    void bindTexture(UInt target, UInt index);

    void texImage2D(UInt target, int level, int internalformat,
                    int width, int height, int border, UInt format,
                    UInt type, const UByte * pixels);

    void texSubImage2D(UInt target, int level, int xOffset, int yOffset,
                       int width, int height, UInt format, UInt type,
                       const UByte * pixels);

private:

    std::vector<Image> images;
};

//FIXME TEMP
extern ImageManager g_ImageMgr;

} // namespace War3 {}

#endif // WAR3_IMAGE_HPP
