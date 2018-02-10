
#pragma once

// ============================================================================
// File:   Renderer.hpp
// Author: Guilherme R. Lampert
// Brief:  "Manager of all managers" related to OpenGL/rendering. A CEO maybe?
// ============================================================================

#include "Image.hpp"
#include "Framebuffer.hpp"
#include "ShaderProgram.hpp"
#include "GLProxy/GLDllUtils.hpp"

namespace War3
{

class Renderer final
{
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

public:
    GLProxy::OpenGLDll&   glDll;
    ImageManager&         imageMgr;
    ShaderProgramManager& shaderProgMgr;
    FramebufferManager&   framebufferMgr;
    bool                  isEnabled;

public:
    static Renderer& getInstance();

    void start();
    void stop();

    void beginFrame();
    void endFrame();
};

} // namespace War3
