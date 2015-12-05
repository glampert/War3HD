
// ================================================================================================
// -*- C++ -*-
// File: Image.cpp
// Author: Guilherme R. Lampert
// Created on: 25/11/15
// Brief: Texture/Image helper classes.
// ================================================================================================

#include "War3/Image.hpp"
#include "GLProxy/GLExtensions.hpp"

// ========================================================
// STB Image Write (stbiw):
// ========================================================

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace War3
{

// ========================================================
// OpenGL texture helpers:
// ========================================================

void bindGLTexture(const UInt target, const UInt texHandle, const int tmu) noexcept
{
    //TODO state caching
    if (tmu >= 0)
    {
        GLProxy::glActiveTexture(GL_TEXTURE0 + tmu);
    }
    GLProxy::glBindTexture(target, texHandle);
}

void setGLTextureFiltering(const UInt target, const Image::Filter filter, const bool withMipmaps) noexcept
{
    // NOTE: The following applies to the currently bound texture!
    switch (filter)
    {
    case Image::Filter::Default :
        //TODO set global default
        break;

    case Image::Filter::Nearest :
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;

    case Image::Filter::Bilinear :
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case Image::Filter::Trilinear :
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case Image::Filter::Anisotropic :
        // Anisotropic filtering only works for mipmaped textures.
        // If not mipmaped, fall back to a linear filter.
        if (withMipmaps)
        {
            GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //TODO get value from renderer
            //glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<GLfloat>(anisotropy));
        }
        else
        {
            GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //TODO
            //GLProxy::glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
        }
        break;

    default :
        fatalError("Invalid GL texture filter!");
    } // switch (filter)

    GLPROXY_CHECK_GL_ERRORS();
}

void setGLPixelAlignment(const UInt packAlign, const UInt width, const UInt bytesPerPix) noexcept
{
    if (packAlign != GL_PACK_ALIGNMENT && packAlign != GL_UNPACK_ALIGNMENT)
    {
        warning("Invalid pixel pack enum!");
        return;
    }

    const UInt rowSizeBytes = width * bytesPerPix;

    // Set the row alignment to the highest value that
    // the size of a row divides evenly. Options are: 8,4,2,1.
    if ((rowSizeBytes % 8) == 0)
    {
        GLProxy::glPixelStorei(packAlign, 8);
    }
    else if ((rowSizeBytes % 4) == 0)
    {
        GLProxy::glPixelStorei(packAlign, 4);
    }
    else if ((rowSizeBytes % 2) == 0)
    {
        GLProxy::glPixelStorei(packAlign, 2);
    }
    else
    {
        GLProxy::glPixelStorei(packAlign, 1);
    }

    GLPROXY_CHECK_GL_ERRORS();
}

Image::TargetUsage targetUsageFromGLEnum(const UInt target) noexcept
{
    switch (target)
    {
    case GL_TEXTURE_2D : return Image::TargetUsage::Texture2D;
    default : fatalError("Bad GLenum for texture target!");
    } // switch (target)
}

Image::PixelFormat pixelFormatFromGLEnum(const UInt internal, const UInt format, const UInt type) noexcept
{
    if (type == GL_UNSIGNED_BYTE)
    {
        if (internal == GL_RGBA || format == GL_RGBA)
        {
            return Image::PixelFormat::RGBA_8888;
        }
        if (internal == GL_LUMINANCE || format == GL_RED || format == GL_DEPTH_COMPONENT)
        {
            return Image::PixelFormat::Grayscale;
        }
    }
    fatalError("Bad GL texture format! Unsupported.");
}

// ========================================================
// class Image:
// ========================================================

Image::Image(PixelBuffer && pb, const PixelFormat fmt, const TargetUsage usage,
             const int idx, const int w, const int h, const int lvl) noexcept
    : pixels { std::move(pb) }
    , format { fmt   }
    , target { usage }
    , index  { idx   }
    , width  { w     }
    , height { h     }
    , level  { lvl   }
{
}

Image::Image(Image && other) noexcept
    : pixels { std::move(other.pixels) }
    , format { other.format }
    , target { other.target }
    , index  { other.index  }
    , width  { other.width  }
    , height { other.height }
    , level  { other.level  }
{
    other.invalidate();
}

Image & Image::operator = (Image && other) noexcept
{
    pixels = std::move(other.pixels);
    format = other.format;
    target = other.target;
    index  = other.index;
    width  = other.width;
    height = other.height;
    level  = other.level;

    other.invalidate();
    return *this;
}

void Image::invalidate() noexcept
{
    pixels.clear();
    format = PixelFormat::Null;
    target = TargetUsage::Null;
    index  = -1;
    width  = -1;
    height = -1;
    level  = -1;
}

bool Image::savePng(const std::string & filename) const noexcept
{
    if (filename.empty())
    {
        warning("No filename provided for Image::savePng()!");
        return false;
    }
    if (!isValid())
    {
        warning("Invalid image in Image::savePng()!");
        return false;
    }

    const int bytesPerPixel = static_cast<int>(format);
    const int strideInBytes = width * bytesPerPixel;

    const int result = stbi_write_png(
            filename.c_str(), width, height,
            bytesPerPixel, pixels.data(), strideInBytes);

    return result != 0;
}

bool Image::saveTga(const std::string & filename) const noexcept
{
    if (filename.empty())
    {
        warning("No filename provided for Image::saveTga()!");
        return false;
    }
    if (!isValid())
    {
        warning("Invalid image in Image::saveTga()!");
        return false;
    }

    const int result = stbi_write_tga(
            filename.c_str(), width, height,
            static_cast<int>(format), pixels.data());

    return result != 0;
}

// ========================================================
// class ImageManager:
// ========================================================

ImageManager::~ImageManager()
{
    saveAllImagesToFile();
}

//FIXME have to mkdir!
void ImageManager::saveAllImagesToFile() const
{
    const std::string baseDir{ "CapturedImages\\" }; //TODO probably place images and logs into a War3HD folder...
    std::string generatedFilename;

    for (const auto & img : images)
    {
        // Only save the first mipmap.
        if (img.getLevel() > 0)
        {
            continue;
        }

        // img_<idx>_<level>_<w>x<h>.png
        generatedFilename  = baseDir;
        generatedFilename += "img_";
        generatedFilename += std::to_string(img.getIndex())  + "_";
        generatedFilename += std::to_string(img.getLevel())  + "_";
        generatedFilename += std::to_string(img.getWidth())  + "x";
        generatedFilename += std::to_string(img.getHeight()) + ".png";

        img.savePng(generatedFilename);
    }
}

void ImageManager::genTextures(int n, UInt * indexes)
{
    (void)n;(void)indexes;
    //TODO
}

void ImageManager::deleteTextures(int n, const UInt * indexes)
{
    (void)n;(void)indexes;
    //TODO
}

void ImageManager::bindTexture(UInt target, UInt index)
{
    (void)target;(void)index;
    //TODO
}

//FIXME generate GL_ERRORS if params are bad???
void ImageManager::texImage2D(UInt target, int level, int internalformat,
                              int width, int height, int border, UInt format,
                              UInt type, const UByte * pixels)
{
    if (border != 0)
    {
        fatalError("Border not zero!"); // FIXME GL_ERROR instead???
    }

    const auto usage = targetUsageFromGLEnum(target);
    const auto pfmt  = pixelFormatFromGLEnum(internalformat, format, type);

    const auto sizeBytes = width * height * static_cast<UInt>(pfmt);
    Image::PixelBuffer pixBuf(pixels, pixels + sizeBytes);

    images.emplace_back(std::move(pixBuf), pfmt, usage,
            static_cast<int>(images.size()), width, height, level);
}

void ImageManager::texSubImage2D(UInt target, int level, int xOffset, int yOffset,
                                 int width, int height, UInt format, UInt type,
                                 const UByte * pixels)
{
    (void)target;(void)level;
    (void)xOffset;(void)yOffset;
    (void)width;(void)height;(void)format;
    (void)type;(void)pixels;
    //TODO
}

//FIXME temp
ImageManager g_ImageMgr;

} // namespace War3 {}
