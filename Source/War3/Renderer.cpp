
// ============================================================================
// File:   Renderer.cpp
// Author: Guilherme R. Lampert
// Brief:  "Manager of all managers" related to OpenGL/rendering. A CEO maybe?
// ============================================================================

#include "Renderer.hpp"
#include "Window.hpp"

namespace War3
{

Renderer::Renderer()
    : glDll{ GLProxy::OpenGLDll::getInstance() }
    , imageMgr{ ImageManager::getInstance() }
    , shaderProgMgr{ ShaderProgramManager::getInstance() }
    , framebufferMgr{ FramebufferManager::getInstance() }
    , isEnabled{ false }
{
    GLProxy::initializeExtensions();
    Window::installDebugHooks();
}

Renderer::~Renderer()
{
    info("---- War3::Renderer shutdown ----\n");

    FramebufferManager::deleteInstance();
    ShaderProgramManager::deleteInstance();
    ImageManager::deleteInstance();
}

Renderer& Renderer::getInstance()
{
    // One-time static initialization that should only happen when the application
    // starts will take place in the singleton constructor.
    static Renderer s_TheRenderer;
    return s_TheRenderer;
}

// One-time initialization/setup when we switch on the custom renderer.
void Renderer::start()
{
    isEnabled = true;
}

// Cleanup when the custom renderer is disabled and we switch back to the original mode.
void Renderer::stop()
{
    isEnabled = false;

    // Make sure logs are done in case we quit after this.
    #if GLPROXY_WITH_LOG
    GLProxy::getProxyDllLogStream().flush();
    #endif // GLPROXY_WITH_LOG

    #if WAR3_WITH_LOG
    War3::getLogStream().flush();
    #endif // WAR3_WITH_LOG
}

// Begin rendering of a custom War3 frame.
void Renderer::beginFrame()
{
    if (!isEnabled)
    {
        return;
    }

    // TODO
}

// End rendering of a custom War3 frame.
void Renderer::endFrame()
{
    if (!isEnabled)
    {
        return;
    }

    // TODO
}

} // namespace War3
