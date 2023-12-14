
#pragma once

// ============================================================================
// File:   Framebuffer.hpp
// Author: Guilherme R. Lampert
// Brief:  Framebuffer capture and management.
// ============================================================================

#include "War3/Common.hpp"
#include "War3/Image.hpp"
#include <array>

namespace War3
{

// ========================================================
// class Framebuffer:
// ========================================================

class Framebuffer final
{
public:
    enum RenderTargetId
    {
        kColorBuffer,
        kDepthBuffer,

        kRTCount // Number of entries in the enum - internal use
    };

    // Not copyable.
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Movable.
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    // Create framebuffer with given dimensions in pixels.
    Framebuffer(int w, int h, bool withDepthBuffer,
                Image::Filter colorFilter, Image::Filter depthFilter);

    // Frees the associated data.
    ~Framebuffer();

    // Binds the framebuffer for draw+read.
    void bind() const noexcept;

    // Bind one of the render target texture for use as a normal OpenGL texture.
    void bindRenderTargetTexture(RenderTargetId rtId, int tmu) const noexcept;

    // Binds the default screen framebuffer (0).
    static void bindNull() noexcept;

    // Handle to the currently enabled GL framebuffer.
    static unsigned getCurrentGLFramebuffer() noexcept { return sm_currentFbo; }

    // Saving render targets to file (path must already exist! / FB must be already bound!):
    bool savePng(const std::string& filename, RenderTargetId rtId) const;
    bool saveTga(const std::string& filename, RenderTargetId rtId) const;

    // Miscellaneous:
    int getWidth()  const noexcept { return m_width; }
    int getHeight() const noexcept { return m_height; }
    bool isValid()  const noexcept { return m_handle != 0 && m_validationOk; }
    bool isBound()  const noexcept { return m_handle == sm_currentFbo; }

private:
    bool saveImageHelper(const std::string& filename, RenderTargetId rtId,
                         bool (Image::*saveMethod)(const std::string&) const) const;

    bool createFramebufferColorTexture(int w, int h, Image::Filter filter) noexcept;
    bool createFramebufferDepthTexture(int w, int h, Image::Filter filter) noexcept;

    void freeGLRenderTargets() noexcept;
    void releaseGLHandles() noexcept;
    void validateSelf() noexcept;
    void invalidate() noexcept;

private:
    // OpenGL FBO handle.
    unsigned m_handle;

    // Dimensions in pixels of all attachments.
    int m_width;
    int m_height;

    // Render target attachments for the FBO. Each is a handle to
    // a GL texture. For an unused attachment, the slot is set to zero.
    std::array<unsigned, kRTCount> m_renderTargets;

    // True if the GL framebuffer validation succeeded.
    bool m_validationOk;

    // Current OpenGL FBO enabled or 0 for the default.
    static unsigned sm_currentFbo;
};

// ========================================================
// class FramebufferManager:
// ========================================================

class FramebufferManager final
{
public:
    FramebufferManager(const FramebufferManager&) = delete;
    FramebufferManager& operator=(const FramebufferManager&) = delete;

    static FramebufferManager& getInstance()
    {
        if (sm_sharedInstance == nullptr)
        {
            sm_sharedInstance = new FramebufferManager{};
        }
        return *sm_sharedInstance;
    }

    static void deleteInstance()
    {
        Framebuffer::bindNull();

        delete sm_sharedInstance;
        sm_sharedInstance = nullptr;
    }

    void onFrameStarted(const Size2D& screenSize);
    void onFrameEnded();

private:
    void presentFrameBuffer();
    void drawFullscreenQuadrilateral();

    // Singleton instance:
    FramebufferManager();
    static FramebufferManager* sm_sharedInstance;

    // The offscreen FB we redirect the War3 renderer to:
    std::unique_ptr<Framebuffer> m_framebuffer;
};

} // namespace War3
