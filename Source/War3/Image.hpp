
#pragma once

// ============================================================================
// File:   Image.hpp
// Author: Guilherme R. Lampert
// Brief:  Texture/Image helper classes.
// ============================================================================

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
    using PixelBuffer = std::vector<std::uint8_t>;

    enum class TargetUsage : std::uint8_t
    {
        kNull = 0,
        kTexture2D
    };

    enum class PixelFormat : std::uint8_t
    {
        kNull = 0,
        kGrayscale = 1, // Depth buffer textures
        kRGBA_8888 = 4  // All textures used by the game
    };

    enum class Filter : std::uint8_t
    {
        kDefault,    // Current default filter defined by the renderer.
        kNearest,    // Nearest neighbor (or Manhattan Distance) filtering. Worst quality, best performance.
        kBilinear,   // Cheap bi-linear filtering. Low quality but good performance.
        kTrilinear,  // Intermediate tri-linear filtering. Reasonable quality, average performance.
        kAnisotropic // Anisotropic filtering. Best quality, most expensive. Paired with an anisotropy amount.
    };

    Image(PixelBuffer&& pb, PixelFormat fmt, TargetUsage usage,
          int idx, int w, int h, int lvl) noexcept;

    // Not copyable.
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    // Movable.
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    // Saving to file (path must already exist!):
    bool savePng(const std::string& filename) const noexcept;
    bool saveTga(const std::string& filename) const noexcept;

    const PixelBuffer& getPixels() const noexcept { return m_pixels; }
    PixelFormat getFormat() const noexcept { return m_format; }
    TargetUsage getTarget() const noexcept { return m_target; }

    int getIndex()  const noexcept { return m_index;  }
    int getWidth()  const noexcept { return m_width;  }
    int getHeight() const noexcept { return m_height; }
    int getLevel()  const noexcept { return m_level;  }

    bool isValid() const noexcept
    {
        return (m_width > 0 && m_height > 0 && m_format != PixelFormat::kNull && !m_pixels.empty());
    }

private:
    void invalidate() noexcept;

    PixelBuffer m_pixels;
    PixelFormat m_format;
    TargetUsage m_target;

    int m_index;
    int m_width;
    int m_height;
    int m_level;
};

// ========================================================
// OpenGL texture helpers:
// ========================================================

namespace GLUtil
{

void bindGLTexture(unsigned target, unsigned texHandle, int tmu = -1);
void setGLTextureFiltering(unsigned target, Image::Filter filter, bool withMipmaps);
void setGLPixelAlignment(unsigned packAlign, unsigned width, unsigned bytesPerPix);

Image::TargetUsage targetUsageFromGLEnum(unsigned target);
Image::PixelFormat pixelFormatFromGLEnum(unsigned internal, unsigned format, unsigned type);

} // namespace GLUtil

// ========================================================
// class ImageManager:
// ========================================================

class ImageManager final
{
public:
    ~ImageManager();
    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(const ImageManager&) = delete;

    static ImageManager& getInstance()
    {
        if (sm_sharedInstance == nullptr)
        {
            sm_sharedInstance = new ImageManager{};
        }
        return *sm_sharedInstance;
    }

    static void deleteInstance()
    {
        delete sm_sharedInstance;
        sm_sharedInstance = nullptr;
    }

    void saveAllImagesToFile() const;

    //
    // Intercept OpenGL texture functions:
    //

    void genTextures(int n, unsigned* indexes);
    void deleteTextures(int n, const unsigned* indexes);
    void bindTexture(unsigned target, unsigned index);

    void texImage2D(unsigned target, int level, int internalformat,
                    int width, int height, int border, unsigned format,
                    unsigned type, const std::uint8_t* pixels);

    void texSubImage2D(unsigned target, int level, int xOffset, int yOffset,
                       int width, int height, unsigned format, unsigned type,
                       const std::uint8_t* pixels);

private:
    // Singleton instance:
    ImageManager();
    static ImageManager* sm_sharedInstance;

    // Active GL images:
    std::vector<Image> m_images;
};

} // namespace War3
