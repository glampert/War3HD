
// ================================================================================================
// -*- C++ -*-
// File: Framebuffer.cpp
// Author: Guilherme R. Lampert
// Created on: 02/12/15
// Brief: Framebuffer capture and management.
// ================================================================================================

#include "War3/Framebuffer.hpp"
#include "War3/ShaderProgram.hpp"
#include "GLProxy/GLExtensions.hpp"

namespace War3
{

// ========================================================
// class Framebuffer:
// ========================================================

UInt Framebuffer::currentFbo{};

Framebuffer::Framebuffer(Framebuffer && other) noexcept
    : handle { other.handle }
    , width  { other.width  }
    , height { other.height }
    , validationOk { other.validationOk }
{
    renderTargets = other.renderTargets;
    other.invalidate();
}

Framebuffer & Framebuffer::operator = (Framebuffer && other) noexcept
{
    handle        = other.handle;
    width         = other.width;
    height        = other.height;
    validationOk  = other.validationOk;
    renderTargets = other.renderTargets;

    other.invalidate();
    return *this;
}

Framebuffer::Framebuffer(const int w, const int h, const bool withDepthBuffer,
                         const Image::Filter colorFilter, const Image::Filter depthFilter)
    : handle {  0 }
    , width  { -1 }
    , height { -1 }
    , validationOk { false }
{
    renderTargets.fill(0);
    GLProxy::initializeExtensions();

    if (w <= 0 || h <= 0)
    {
        warning("Bad Framebuffer dimensions!");
        return;
    }

    GLuint glFboHandle = 0;
    GLProxy::glGenFramebuffers(1, &glFboHandle);

    if (glFboHandle == 0)
    {
        warning("Failed to allocate a new GL FBO handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
    }

    currentFbo = glFboHandle;
    GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, glFboHandle);

    if (!createFramebufferColorTexture(w, h, colorFilter))
    {
        bindNull();
        freeGLRenderTargets();
        GLProxy::glDeleteFramebuffers(1, &glFboHandle);

        warning("Failed to allocate one or more Framebuffer color textures!");
        GLPROXY_CHECK_GL_ERRORS();
        return;
    }

    if (withDepthBuffer)
    {
        if (!createFramebufferDepthTexture(w, h, depthFilter))
        {
            bindNull();
            freeGLRenderTargets();
            GLProxy::glDeleteFramebuffers(1, &glFboHandle);

            warning("Failed to allocate Framebuffer depth render target!");
            GLPROXY_CHECK_GL_ERRORS();
            return;
        }
    }

    width  = w;
    height = h;
    handle = glFboHandle;

    validateSelf();
    GLPROXY_CHECK_GL_ERRORS();

    info("New Framebuffer created: " + std::to_string(width) + "x" + std::to_string(height));
}

bool Framebuffer::createFramebufferColorTexture(const int w, const int h, const Image::Filter filter) noexcept
{
    // ColorBuffer:
    GLuint glTexHandle = 0;
    GLProxy::glGenTextures(1, &glTexHandle);

    if (glTexHandle == 0)
    {
        return false;
    }

    bindGLTexture(GL_TEXTURE_2D, glTexHandle);

    setGLTextureFiltering(GL_TEXTURE_2D, filter, false);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLProxy::glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    GLProxy::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D, glTexHandle, 0);

    bindGLTexture(GL_TEXTURE_2D, 0);

    renderTargets[static_cast<int>(RenderTarget::ColorBuffer)] = glTexHandle;
    return true;
}

bool Framebuffer::createFramebufferDepthTexture(const int w, const int h, const Image::Filter filter) noexcept
{
    // DepthBuffer:
    GLuint glTexHandle = 0;
    GLProxy::glGenTextures(1, &glTexHandle);

    if (glTexHandle == 0)
    {
        return false;
    }

    bindGLTexture(GL_TEXTURE_2D, glTexHandle);

    setGLTextureFiltering(GL_TEXTURE_2D, filter, false);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

    GLProxy::glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, w, h);
    GLProxy::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D, glTexHandle, 0);

    bindGLTexture(GL_TEXTURE_2D, 0);

    renderTargets[static_cast<int>(RenderTarget::DepthBuffer)] = glTexHandle;
    return true;
}

Framebuffer::~Framebuffer()
{
    releaseGLHandles();
}

void Framebuffer::validateSelf() noexcept
{
    if (handle == 0 || !isBound())
    {
        warning("Framebuffer not bound or null in Framebuffer::validateSelf()!");
        validationOk = false;
        return;
    }

    const GLenum status = GLProxy::glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        validationOk = true;
        return;
    }

    switch (status)
    {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
        break;

    case GL_FRAMEBUFFER_UNSUPPORTED :
        warning("Framebuffer error: GL_FRAMEBUFFER_UNSUPPORTED");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS :
        warning("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
        break;

    default :
        warning("Unknown Framebuffer status: " + std::to_string(status));
        break;
    } // switch (status)

    validationOk = false;
}

void Framebuffer::freeGLRenderTargets() noexcept
{
    bindGLTexture(GL_TEXTURE_2D, 0);
    GLProxy::glDeleteTextures(RTCount, renderTargets.data());
    renderTargets.fill(0);
}

void Framebuffer::releaseGLHandles() noexcept
{
    if (handle != 0)
    {
        if (handle == currentFbo)
        {
            bindNull();
        }

        GLProxy::glDeleteFramebuffers(1, & handle);
        validationOk = false;
        handle = 0;

        freeGLRenderTargets();
    }
}

void Framebuffer::invalidate() noexcept
{
    handle        =  0;
    width         = -1;
    height        = -1;
    validationOk  = false;
    renderTargets.fill(0);
}

void Framebuffer::bind() const noexcept
{
    if (!isValid())
    {
        warning("Trying to bind an invalid Framebuffer!");
        bindNull();
        return;
    }

    if (handle != currentFbo)
    {
        currentFbo = handle;
        GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, handle);
    }
}

void Framebuffer::bindRenderTargetTexture(const RenderTarget rtId, const int tmu) const noexcept
{
    if (renderTargets[static_cast<int>(rtId)] == 0)
    {
        warning("RenderTarget texture index is null!");
    }
    bindGLTexture(GL_TEXTURE_2D, renderTargets[static_cast<int>(rtId)], tmu);
}

void Framebuffer::bindNull() noexcept
{
    currentFbo = 0;
    GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Framebuffer::savePng(const std::string & filename, const RenderTarget rtId) const
{
    return saveImageHelper(filename, rtId, &Image::savePng);
}

bool Framebuffer::saveTga(const std::string & filename, const RenderTarget rtId) const
{
    return saveImageHelper(filename, rtId, &Image::saveTga);
}

bool Framebuffer::saveImageHelper(const std::string & filename, const RenderTarget rtId,
                                  bool (Image::*saveMethod)(const std::string &) const) const
{
    if (!isBound() || !isValid())
    {
        warning("Can't save invalid/unbound Framebuffer to file!");
        return false;
    }

    const UInt maxBytesPerPixel = 4; // RGBA is the largest format
    const std::size_t sizeBytes = width * height * maxBytesPerPixel;

    Image::PixelFormat imageFmt;
    Image::PixelBuffer pixBuf(sizeBytes);

    if (rtId != RenderTarget::DepthBuffer)
    {
        imageFmt = Image::PixelFormat::RGBA_8888;
        setGLPixelAlignment(GL_PACK_ALIGNMENT, width, 4);

        bindGLTexture(GL_TEXTURE_2D, renderTargets[static_cast<int>(rtId)]);
        GLProxy::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixBuf.data());
        bindGLTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        imageFmt = Image::PixelFormat::Grayscale;
        setGLPixelAlignment(GL_PACK_ALIGNMENT, width, 1);

        // Use glReadPixels because it provides automatic
        // conversion from the depth format to grayscale.
        GLProxy::glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT,
                              GL_UNSIGNED_BYTE, pixBuf.data());
    }

    GLPROXY_CHECK_GL_ERRORS();

    Image img{ std::move(pixBuf), imageFmt, Image::TargetUsage::Texture2D, 0, width, height, 0 };
    return (img.*saveMethod)(filename);
}

// ========================================================
// class FramebufferManager:
// ========================================================

FramebufferManager * FramebufferManager::sharedInstance{ nullptr };

FramebufferManager::FramebufferManager()
{
    info("---- FramebufferManager startup ----");
}

void FramebufferManager::onFrameStarted(const int scrW, const int scrH)
{
    // First run or screen resolution changed? Recreate the FB.
    if (framebuffer == nullptr || framebuffer->getWidth() != scrW || framebuffer->getHeight() != scrH)
    {
        // Make sure the current is freed first to
        // avoid having both in memory at the same time.
        framebuffer = nullptr;

        if (scrW > 0 && scrH > 0)
        {
            framebuffer = std::make_unique<Framebuffer>(scrW, scrH, true,
                        Image::Filter::Bilinear, Image::Filter::Nearest);
        }
        else
        {
            error("Zero/negative Framebuffer dimensions in onFrameStarted()!");
            return;
        }
    }

    framebuffer->bind();
}

void FramebufferManager::onFrameEnded()
{
    if (framebuffer == nullptr)
    {
        return;
    }

    Framebuffer::bindNull();
    presentColorBuffer();
}

void FramebufferManager::presentColorBuffer()
{
    //TODO TEMP testing
    const auto & sp = ShaderProgramManager::getInstance().getShader(ShaderProgramManager::ShaderId::FramePostProcess);
    sp.bind();

    sp.setUniform1i(sp.getUniformLocation("u_ColorRenderTarget"), 0);
    framebuffer->bindRenderTargetTexture(Framebuffer::RenderTarget::ColorBuffer, 0);

    drawNdcQuadrilateral();
    GLPROXY_CHECK_GL_ERRORS();
    ShaderProgram::bindNull();

    bindGLTexture(GL_TEXTURE_2D, 0, 0);
}

void FramebufferManager::drawNdcQuadrilateral()
{
    GLProxy::glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GLProxy::glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    GLProxy::glEnableClientState(GL_VERTEX_ARRAY);
    GLProxy::glDisable(GL_DEPTH_TEST);
    GLProxy::glDisable(GL_BLEND);

    static const float verts[]{
        // First triangle:
         1.0,  1.0,
        -1.0,  1.0,
        -1.0, -1.0,
        // Second triangle:
        -1.0, -1.0,
         1.0, -1.0,
         1.0,  1.0
    };

    GLProxy::glVertexPointer(2, GL_FLOAT, 0, verts);
    GLProxy::glDrawArrays(GL_TRIANGLES, 0, 6);

    GLProxy::glPopClientAttrib();
    GLProxy::glPopAttrib();
}

} // namespace War3 {}
