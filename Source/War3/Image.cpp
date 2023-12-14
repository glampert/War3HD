
// ============================================================================
// File:   Image.cpp
// Author: Guilherme R. Lampert
// Brief:  Texture/Image helper classes.
// ============================================================================

#include "Image.hpp"
#include "DebugUI.hpp"
#include "GLProxy/GLExtensions.hpp"

// STB Image Write (stbiw):
#define STBIW_ASSERT(expr) WAR3_ASSERT(expr)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace War3
{

// ========================================================
// OpenGL texture helpers:
// ========================================================

namespace GLUtil
{

void bindGLTexture(const unsigned target, const unsigned texHandle, const int tmu)
{
    // TODO: GL state caching?
    if (tmu >= 0)
    {
        GLProxy::glActiveTexture(GL_TEXTURE0 + tmu);
    }
    GLProxy::glBindTexture(target, texHandle);
}

void setGLTextureFiltering(const unsigned target, const Image::Filter filter, const bool withMipmaps)
{
    // NOTE: The following applies to the currently bound texture!
    switch (filter)
    {
    case Image::Filter::kDefault:
        // TODO: Set global default
        warn("Image::Filter::kDefault UNIMPLEMENTED!");
        break;

    case Image::Filter::kNearest:
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;

    case Image::Filter::kBilinear:
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case Image::Filter::kTrilinear:
        GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (withMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
        GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case Image::Filter::kAnisotropic:
        // Anisotropic filtering only works for mipmaped textures.
        // If not mipmaped, fall back to a linear filter.
        if (withMipmaps)
        {
            GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // TODO: get value from renderer
            //GLProxy::glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<GLfloat>(anisotropy));
        }
        else
        {
            GLProxy::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GLProxy::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // TODO: get value from renderer
            //GLProxy::glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
        }
        break;

    default:
        fatalError("Invalid GL texture filter!");
    } // switch (filter)

    GLPROXY_CHECK_GL_ERRORS();
}

void setGLPixelAlignment(const unsigned packAlign, const unsigned width, const unsigned bytesPerPix)
{
    if (packAlign != GL_PACK_ALIGNMENT && packAlign != GL_UNPACK_ALIGNMENT)
    {
        warn("Invalid pixel pack enum!");
        return;
    }

    const unsigned rowSizeBytes = width * bytesPerPix;

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

Image::TargetUsage targetUsageFromGLEnum(const unsigned target)
{
    switch (target)
    {
    case GL_TEXTURE_2D:
        return Image::TargetUsage::kTexture2D;
    default:
        fatalError("Bad GLenum for texture target!");
    } // switch (target)
}

Image::PixelFormat pixelFormatFromGLEnum(const unsigned internal, const unsigned format, const unsigned type)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        if (internal == GL_RGBA || format == GL_RGBA)
        {
            return Image::PixelFormat::kRGBA_8888;
        }
        if (internal == GL_LUMINANCE || format == GL_RED || format == GL_DEPTH_COMPONENT)
        {
            return Image::PixelFormat::kGrayscale;
        }
    }
    fatalError("Bad GL texture format! Unsupported.");
}

} // namespace GLUtil

// ========================================================
// class Image:
// ========================================================

Image::Image(PixelBuffer&& pb, const PixelFormat fmt, const TargetUsage usage,
             const int idx, const int w, const int h, const int lvl) noexcept
    : m_pixels{ std::move(pb) }
    , m_format{ fmt }
    , m_target{ usage }
    , m_index{ idx }
    , m_width{ w }
    , m_height{ h }
    , m_level{ lvl }
{
}

Image::Image(Image&& other) noexcept
    : m_pixels{ std::move(other.m_pixels) }
    , m_format{ other.m_format }
    , m_target{ other.m_target }
    , m_index{ other.m_index }
    , m_width{ other.m_width }
    , m_height{ other.m_height }
    , m_level{ other.m_level }
{
    other.invalidate();
}

Image& Image::operator=(Image&& other) noexcept
{
    m_pixels = std::move(other.m_pixels);
    m_format = other.m_format;
    m_target = other.m_target;
    m_index  = other.m_index;
    m_width  = other.m_width;
    m_height = other.m_height;
    m_level  = other.m_level;

    other.invalidate();
    return *this;
}

void Image::invalidate() noexcept
{
    m_pixels.clear();
    m_format = PixelFormat::kNull;
    m_target = TargetUsage::kNull;
    m_index  = -1;
    m_width  = -1;
    m_height = -1;
    m_level  = -1;
}

bool Image::savePng(const std::string& filename) const noexcept
{
    if (filename.empty())
    {
        warn("No filename provided for Image::savePng()!");
        return false;
    }
    if (!isValid())
    {
        warn("Invalid image in Image::savePng()!");
        return false;
    }

    const int bytesPerPixel = static_cast<int>(m_format);
    const int strideInBytes = m_width * bytesPerPixel;

    const int result = stbi_write_png(filename.c_str(), m_width, m_height,
                                      bytesPerPixel, m_pixels.data(), strideInBytes);
    return result != 0;
}

bool Image::saveTga(const std::string& filename) const noexcept
{
    if (filename.empty())
    {
        warn("No filename provided for Image::saveTga()!");
        return false;
    }
    if (!isValid())
    {
        warn("Invalid image in Image::saveTga()!");
        return false;
    }

    const int result = stbi_write_tga(filename.c_str(), m_width, m_height,
                                      static_cast<int>(m_format), m_pixels.data());
    return result != 0;
}

// ========================================================
// class ImageManager:
// ========================================================

ImageManager* ImageManager::sm_sharedInstance{ nullptr };

ImageManager::ImageManager()
{
    info("---- ImageManager startup ----");
}

ImageManager::~ImageManager()
{
    if (!m_images.empty() && DebugUI::dumpTexturesToFile)
    {
        saveAllImagesToFile();
    }
}

void ImageManager::saveAllImagesToFile() const
{
    const std::string baseDir{ "CapturedImages\\" }; // TODO: probably place images and logs into a War3HD folder...
    std::string generatedFilename;

    createDirectories(baseDir);

    int numSaved = 0;
    for (const auto& img : m_images)
    {
        // Only save the first mipmap.
        if (img.getLevel() > 0)
        {
            continue;
        }

        // img_<idx>_<level>_<w>x<h>.png
        generatedFilename = baseDir;
        generatedFilename += "img_";
        generatedFilename += std::to_string(img.getIndex()) + "_";
        generatedFilename += std::to_string(img.getLevel()) + "_";
        generatedFilename += std::to_string(img.getWidth()) + "x";
        generatedFilename += std::to_string(img.getHeight()) + ".png";

        img.savePng(generatedFilename);
        ++numSaved;
    }

    info("Saved %i textures to file.", numSaved);
}

void ImageManager::genTextures(int n, unsigned* indexes)
{
    (void)n;
    (void)indexes;
    // TODO
}

void ImageManager::deleteTextures(int n, const unsigned* indexes)
{
    (void)n;
    (void)indexes;
    // TODO
}

void ImageManager::bindTexture(unsigned target, unsigned index)
{
    (void)target;
    (void)index;
    // TODO
}

void ImageManager::texImage2D(unsigned target, int level, int internalformat,
                              int width, int height, int border, unsigned format,
                              unsigned type, const std::uint8_t* pixels)
{
    if (border != 0)
    {
        fatalError("Border not zero!"); // TODO: should we generate a GL_ERROR instead???
    }

    const auto usage = GLUtil::targetUsageFromGLEnum(target);
    const auto pfmt  = GLUtil::pixelFormatFromGLEnum(internalformat, format, type);

    const auto sizeBytes = width * height * static_cast<unsigned>(pfmt);
    Image::PixelBuffer pixBuf(pixels, pixels + sizeBytes);

    m_images.emplace_back(std::move(pixBuf), pfmt, usage,
                          static_cast<int>(m_images.size()), width, height, level);
}

void ImageManager::texSubImage2D(unsigned target, int level, int xOffset, int yOffset,
                                 int width, int height, unsigned format, unsigned type,
                                 const std::uint8_t* pixels)
{
    (void)target;
    (void)level;
    (void)xOffset;
    (void)yOffset;
    (void)width;
    (void)height;
    (void)format;
    (void)type;
    (void)pixels;
    // TODO
}

} // namespace War3
