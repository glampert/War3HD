
// ============================================================================
// File:   Renderer.cpp
// Author: Guilherme R. Lampert
// Brief:  "Manager of all managers" related to OpenGL/rendering. A CEO maybe?
// ============================================================================

#include "Renderer.hpp"
#include "Window.hpp"
#include "DebugUI.hpp"

namespace War3
{

Renderer::Renderer()
    : glDll{ GLProxy::OpenGLDll::getInstance() }
{
    info("---- War3::Renderer startup ----");
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
    info("=--=--=--=--=--=--=--=--=--=--=--=--=");
    info("------ War3::Renderer::start() ------");
    info("=--=--=--=--=--=--=--=--=--=--=--=--=");

    isEnabled = true;
    GLProxy::loadInternalGLFunctions();

    // Back from minimizing/maximizing the window, set debug size.
    if (Window::getHandle())
    {
        Window::setWindowed(Window::kDefaultDebugSize.width,
                            Window::kDefaultDebugSize.height);
    }

    auto versionStr  = (const char*)GLProxy::glGetString(GL_VERSION);
    auto vendorStr   = (const char*)GLProxy::glGetString(GL_VENDOR);
    auto rendererStr = (const char*)GLProxy::glGetString(GL_RENDERER);

    info("GL_VERSION  = %s", versionStr);
    info("GL_VENDOR   = %s", vendorStr);
    info("GL_RENDERER = %s", rendererStr);

    DebugUI::start();
}

// Cleanup when the custom renderer is disabled and we switch back to the original mode.
void Renderer::stop()
{
    info("=--=--=--=--=--=--=--=--=--=--=--=--=");
    info("------ War3::Renderer::stop() -------");
    info("=--=--=--=--=--=--=--=--=--=--=--=--=");

    isEnabled = false;
    DebugUI::stop();

    // Recreate all singletons.
    FramebufferManager::deleteInstance();
    ShaderProgramManager::deleteInstance();
    ImageManager::deleteInstance();

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

    screenSize = Window::getScreenSize();

    FramebufferManager::getInstance().onFrameStarted(screenSize);

    if (DebugUI::enableDebugShader())
    {
        const auto& sp = ShaderProgramManager::getInstance().getShader<DebugShaderProgram>(ShaderProgramManager::kDebug);
        sp.bind();

        sp.setScreenSize(screenSize);

        if (DebugUI::debugViewTexCoords)
        {
            sp.setDebugView(DebugShaderProgram::kDebugViewTexCoords);
        }
        else if (DebugUI::debugViewVertNormals)
        {
            sp.setDebugView(DebugShaderProgram::kDebugViewVertNormals);
        }
        else if (DebugUI::debugViewVertColors)
        {
            sp.setDebugView(DebugShaderProgram::kDebugViewVertColors);
        }
        else if (DebugUI::debugViewVertPositions)
        {
            sp.setDebugView(DebugShaderProgram::kDebugViewVertPositions);
        }
        else if (DebugUI::debugViewPolyOutlines)
        {
            sp.setDebugView(DebugShaderProgram::kDebugViewPolyOutlines);
        }
    }
}

// End rendering of a custom War3 frame.
void Renderer::endFrame()
{
    if (!isEnabled)
    {
        return;
    }

    ShaderProgram::bindNull();

    FramebufferManager::getInstance().onFrameEnded();

    DebugUI::render(screenSize);
}

} // namespace War3
