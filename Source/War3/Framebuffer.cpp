
// ============================================================================
// File:   Framebuffer.cpp
// Author: Guilherme R. Lampert
// Brief:  Framebuffer capture and management.
// ============================================================================

#include "War3/Framebuffer.hpp"
#include "War3/ShaderProgram.hpp"
#include "GLProxy/GLExtensions.hpp"

namespace War3
{

// ========================================================
// class Framebuffer:
// ========================================================

unsigned Framebuffer::sm_currentFbo{};

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_handle{ other.m_handle }
    , m_width{ other.m_width }
    , m_height{ other.m_height }
    , m_validationOk{ other.m_validationOk }
{
    m_renderTargets = other.m_renderTargets;
    other.invalidate();
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    releaseGLHandles();

    m_handle = other.m_handle;
    m_width = other.m_width;
    m_height = other.m_height;
    m_validationOk = other.m_validationOk;
    m_renderTargets = other.m_renderTargets;

    other.invalidate();
    return *this;
}

Framebuffer::Framebuffer(const int w, const int h, const bool withDepthBuffer,
                         const Image::Filter colorFilter, const Image::Filter depthFilter)
    : m_handle{ 0 }
    , m_width{ -1 }
    , m_height{ -1 }
    , m_validationOk{ false }
{
    m_renderTargets.fill(0);
    GLProxy::initializeExtensions();

    if (w <= 0 || h <= 0)
    {
        warn("Bad Framebuffer dimensions!");
        return;
    }

    GLuint glFboHandle = 0;
    GLProxy::glGenFramebuffers(1, &glFboHandle);

    if (glFboHandle == 0)
    {
        warn("Failed to allocate a new GL FBO handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
    }

    sm_currentFbo = glFboHandle;
    GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, glFboHandle);

    if (!createFramebufferColorTexture(w, h, colorFilter))
    {
        bindNull();
        freeGLRenderTargets();
        GLProxy::glDeleteFramebuffers(1, &glFboHandle);

        warn("Failed to allocate one or more Framebuffer color textures!");
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

            warn("Failed to allocate Framebuffer depth render target!");
            GLPROXY_CHECK_GL_ERRORS();
            return;
        }
    }

    m_width  = w;
    m_height = h;
    m_handle = glFboHandle;

    validateSelf();
    GLPROXY_CHECK_GL_ERRORS();

    info("New Framebuffer created: %dx%d", m_width, m_height);
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

    GLUtil::bindGLTexture(GL_TEXTURE_2D, glTexHandle);

    GLUtil::setGLTextureFiltering(GL_TEXTURE_2D, filter, false);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLProxy::glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    GLProxy::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D, glTexHandle, 0);

    GLUtil::bindGLTexture(GL_TEXTURE_2D, 0);

    m_renderTargets[kColorBuffer] = glTexHandle;
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

    GLUtil::bindGLTexture(GL_TEXTURE_2D, glTexHandle);

    GLUtil::setGLTextureFiltering(GL_TEXTURE_2D, filter, false);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLProxy::glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

    GLProxy::glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, w, h);
    GLProxy::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D, glTexHandle, 0);

    GLUtil::bindGLTexture(GL_TEXTURE_2D, 0);

    m_renderTargets[kDepthBuffer] = glTexHandle;
    return true;
}

Framebuffer::~Framebuffer()
{
    releaseGLHandles();
}

void Framebuffer::validateSelf() noexcept
{
    if (m_handle == 0 || !isBound())
    {
        warn("Framebuffer not bound or null in Framebuffer::validateSelf()!");
        m_validationOk = false;
        return;
    }

    const GLenum status = GLProxy::glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        m_validationOk = true;
        return;
    }

    switch (status)
    {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
        break;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        warn("Framebuffer error: GL_FRAMEBUFFER_UNSUPPORTED");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        warn("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
        break;

    default:
        warn("Unknown Framebuffer status: %u", status);
        break;
    } // switch (status)

    m_validationOk = false;
}

void Framebuffer::freeGLRenderTargets() noexcept
{
    GLUtil::bindGLTexture(GL_TEXTURE_2D, 0);
    GLProxy::glDeleteTextures(kRTCount, m_renderTargets.data());
    m_renderTargets.fill(0);
}

void Framebuffer::releaseGLHandles() noexcept
{
    if (m_handle != 0)
    {
        if (m_handle == sm_currentFbo)
        {
            bindNull();
        }

        GLProxy::glDeleteFramebuffers(1, &m_handle);
        m_validationOk = false;
        m_handle = 0;

        freeGLRenderTargets();
    }
}

void Framebuffer::invalidate() noexcept
{
    m_handle = 0;
    m_width  = -1;
    m_height = -1;
    m_validationOk = false;
    m_renderTargets.fill(0);
}

void Framebuffer::bind() const noexcept
{
    if (!isValid())
    {
        warn("Trying to bind an invalid Framebuffer!");
        bindNull();
        return;
    }

    if (m_handle != sm_currentFbo)
    {
        sm_currentFbo = m_handle;
        GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    }
}

void Framebuffer::bindRenderTargetTexture(const RenderTargetId rtId, const int tmu) const noexcept
{
    if (m_renderTargets[rtId] == 0)
    {
        warn("RenderTarget texture index is null!");
    }
    GLUtil::bindGLTexture(GL_TEXTURE_2D, m_renderTargets[rtId], tmu);
}

void Framebuffer::bindNull() noexcept
{
    sm_currentFbo = 0;
    GLProxy::glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Framebuffer::savePng(const std::string& filename, const RenderTargetId rtId) const
{
    return saveImageHelper(filename, rtId, &Image::savePng);
}

bool Framebuffer::saveTga(const std::string& filename, const RenderTargetId rtId) const
{
    return saveImageHelper(filename, rtId, &Image::saveTga);
}

bool Framebuffer::saveImageHelper(const std::string& filename, const RenderTargetId rtId,
                                  bool (Image::*saveMethod)(const std::string&) const) const
{
    if (!isBound() || !isValid())
    {
        warn("Can't save invalid/unbound Framebuffer to file!");
        return false;
    }

    const unsigned maxBytesPerPixel = 4; // RGBA is the largest format
    const std::size_t sizeBytes = m_width * m_height * maxBytesPerPixel;

    Image::PixelFormat imageFmt;
    Image::PixelBuffer pixBuf(sizeBytes);

    if (rtId != kDepthBuffer)
    {
        imageFmt = Image::PixelFormat::kRGBA_8888;
        GLUtil::setGLPixelAlignment(GL_PACK_ALIGNMENT, m_width, 4);

        GLUtil::bindGLTexture(GL_TEXTURE_2D, m_renderTargets[rtId]);
        GLProxy::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixBuf.data());
        GLUtil::bindGLTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        imageFmt = Image::PixelFormat::kGrayscale;
        GLUtil::setGLPixelAlignment(GL_PACK_ALIGNMENT, m_width, 1);

        // Use glReadPixels because it provides automatic
        // conversion from the depth format to grayscale.
        GLProxy::glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT,
                              GL_UNSIGNED_BYTE, pixBuf.data());
    }

    GLPROXY_CHECK_GL_ERRORS();

    Image img{ std::move(pixBuf), imageFmt, Image::TargetUsage::kTexture2D, 0, m_width, m_height, 0 };
    return (img.*saveMethod)(filename);
}

// ========================================================
// class FramebufferManager:
// ========================================================

FramebufferManager* FramebufferManager::sm_sharedInstance{ nullptr };

FramebufferManager::FramebufferManager()
{
    info("---- FramebufferManager startup ----");
}

void FramebufferManager::onFrameStarted(const int scrW, const int scrH)
{
    // First run or screen resolution changed? Recreate the FB.
    if (m_framebuffer == nullptr || m_framebuffer->getWidth() != scrW || m_framebuffer->getHeight() != scrH)
    {
        // Make sure the current is freed first to
        // avoid having both in memory at the same time.
        m_framebuffer = nullptr;

        if (scrW > 0 && scrH > 0)
        {
            m_framebuffer = std::make_unique<Framebuffer>(scrW, scrH, true,
                                                          Image::Filter::kBilinear,
                                                          Image::Filter::kNearest);
        }
        else
        {
            error("Zero/negative Framebuffer dimensions in onFrameStarted()!");
            return;
        }
    }

    m_framebuffer->bind();
}

void FramebufferManager::onFrameEnded()
{
    if (m_framebuffer == nullptr)
    {
        return;
    }

    Framebuffer::bindNull();
    presentColorBuffer();
}

void FramebufferManager::presentColorBuffer()
{
    //TODO TEMP testing
    const auto& sp = ShaderProgramManager::getInstance().getShader(ShaderProgramManager::kFramePostProcess);
    sp.bind();

    sp.setUniform1i(sp.getUniformLocation("u_ColorRenderTarget"), 0);
    m_framebuffer->bindRenderTargetTexture(Framebuffer::kColorBuffer, 0);

    drawFullscreenQuadrilateral();
    GLPROXY_CHECK_GL_ERRORS();
    ShaderProgram::bindNull();

    GLUtil::bindGLTexture(GL_TEXTURE_2D, 0, 0);
}

void FramebufferManager::drawFullscreenQuadrilateral()
{
    GLProxy::glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GLProxy::glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    GLProxy::glEnableClientState(GL_VERTEX_ARRAY);
    GLProxy::glDisable(GL_DEPTH_TEST);
    GLProxy::glDisable(GL_BLEND);

    static const float s_verts[] = {
        // First triangle:
         1.0,  1.0,
        -1.0,  1.0,
        -1.0, -1.0,
        // Second triangle:
        -1.0, -1.0,
         1.0, -1.0,
         1.0,  1.0,
    };

    GLProxy::glVertexPointer(2, GL_FLOAT, 0, s_verts);
    GLProxy::glDrawArrays(GL_TRIANGLES, 0, 6);

    GLProxy::glPopClientAttrib();
    GLProxy::glPopAttrib();
}

} // namespace War3
