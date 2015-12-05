
// ================================================================================================
// -*- C++ -*-
// File: Framebuffer.hpp
// Author: Guilherme R. Lampert
// Created on: 02/12/15
// Brief: Framebuffer capture and management.
// ================================================================================================

#ifndef WAR3_FRAMEBUFFER_HPP
#define WAR3_FRAMEBUFFER_HPP

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

    enum class RenderTarget
    {
        ColorBuffer,
        DepthBuffer,
        Count // Internal use
    };

    // Not copyable.
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer & operator = (const Framebuffer &) = delete;

    // Movable.
    Framebuffer(Framebuffer && other) noexcept;
    Framebuffer & operator = (Framebuffer && other) noexcept;

    // Create framebuffer with given dimensions in pixels.
    Framebuffer(int w, int h, bool withDepthBuffer,
                Image::Filter colorFilter, Image::Filter depthFilter);

    // Frees the associated data.
    ~Framebuffer();

    // Binds the framebuffer for draw+read.
    void bind() const noexcept;

    // Bind one of the render target texture for use as a normal OpenGL texture.
    void bindRenderTargetTexture(RenderTarget rtId, int tmu) const noexcept;

    // Binds the default screen framebuffer (0).
    static void bindNull() noexcept;

    // Handle to the currently enabled GL framebuffer.
    static UInt getCurrentGLFramebuffer() noexcept { return currentFbo; }

    // Saving render targets to file (path must already exist! / FB must be already bound!):
    bool savePng(const std::string & filename, RenderTarget rtId) const;
    bool saveTga(const std::string & filename, RenderTarget rtId) const;

    // Miscellaneous:
    int getWidth()  const noexcept { return width;  }
    int getHeight() const noexcept { return height; }
    bool isValid()  const noexcept { return handle != 0 && validationOk; }
    bool isBound()  const noexcept { return handle == currentFbo; }

private:

    bool saveImageHelper(const std::string & filename, RenderTarget rtId,
                         bool (Image::*saveMethod)(const std::string &) const) const;

    bool createFramebufferColorTexture(int w, int h, Image::Filter filter) noexcept;
    bool createFramebufferDepthTexture(int w, int h, Image::Filter filter) noexcept;

    void freeGLRenderTargets() noexcept;
    void releaseGLHandles() noexcept;
    void validateSelf() noexcept;
    void invalidate() noexcept;

    // OpenGL FBO handle.
    UInt handle;

    // Dimensions in pixels of all attachments.
    int width;
    int height;

    // Render target attachments for the FBO. Each is a handle to
    // a GL texture. For an unused attachment, the slot is set to zero.
    static constexpr int RTCount = int(RenderTarget::Count);
    std::array<UInt, RTCount> renderTargets;

    // True if the GL framebuffer validation succeeded.
    bool validationOk;

    // Current OpenGL FBO enabled or 0 for the default.
    static UInt currentFbo;
};

// ========================================================
// class FramebufferManager:
// ========================================================

class FramebufferManager final
{
public:

    // Not copyable.
    FramebufferManager(const FramebufferManager &) = delete;
    FramebufferManager & operator = (const FramebufferManager &) = delete;

    static FramebufferManager & getInstance();
    static void deleteInstance();

    void onFrameStarted(int scrW, int scrH);
    void onFrameEnded();

private:

    void presentColorBuffer();
    void drawNdcQuadrilateral();

    // Singleton instance:
    FramebufferManager();
    static FramebufferManager * sharedInstance;

    // The offscreen FB we redirect War3 render to:
    std::unique_ptr<Framebuffer> framebuffer;
};

} // namespace War3 {}

#endif // WAR3_FRAMEBUFFER_HPP
