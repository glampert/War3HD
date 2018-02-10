
// ============================================================================
// File:   GLDllUtils.cpp
// Author: Guilherme R. Lampert
// Brief:  Real OpenGL DLL loader and other DLL interception/stats utils.
// ============================================================================

#include "GLProxy/GLDllUtils.hpp"
#include <algorithm>
#include <vector>

namespace GLProxy
{

// ========================================================
// OpenGLDll:
// ========================================================

OpenGLDll::OpenGLDll() : m_dllHandle{ nullptr }
{
    load();
}

OpenGLDll::~OpenGLDll()
{
    unload();
}

void OpenGLDll::load()
{
    if (isLoaded())
    {
        GLProxy::fatalError("Real OpenGL DLL is already loaded!");
    }

    const std::string glDllFilePath = War3::getRealGLLibPath();
    GLPROXY_LOG("Trying to load real opengl32.dll from \"%s\"...", glDllFilePath.c_str());

    m_dllHandle = LoadLibraryExA(glDllFilePath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (m_dllHandle == nullptr)
    {
        GLProxy::fatalError("GLProxy unable to load the real OpenGL DLL!\n%s", War3::lastWinErrorAsString().c_str());
    }

    const auto selfHMod = War3::getSelfModuleHandle();
    if (m_dllHandle == selfHMod)
    {
        GLProxy::fatalError("GLProxy trying to load itself as the real opengl32.dll!");
    }

    char tempString[1024] = { '\0' };
    if (GetModuleFileNameA(m_dllHandle, tempString, sizeof(tempString)) == 0)
    {
        GLPROXY_LOG("Unable to get Real OpenGL DLL file path!");
    }
    else
    {
        m_dllFilePath = tempString;
    }

    GLPROXY_LOG("\n--------------------------------------------------------");
    GLPROXY_LOG("  Real OpenGL DLL is loaded!");
    GLPROXY_LOG("  OpenGL = %s, GLProxy = %s", War3::ptrToString(m_dllHandle).c_str(), War3::ptrToString(selfHMod).c_str());
    GLPROXY_LOG("  opengl32.dll path: \"%s\"", m_dllFilePath.c_str());
    GLPROXY_LOG("--------------------------------------------------------\n");
}

void OpenGLDll::unload()
{
    if (isLoaded())
    {
        FreeLibrary(m_dllHandle);
        m_dllHandle = nullptr;
        m_dllFilePath.clear();
    }
}

bool OpenGLDll::isLoaded() const noexcept
{
    return m_dllHandle != nullptr;
}

void* OpenGLDll::getFuncPtr(const char* funcName) const
{
    if (!isLoaded())
    {
        GLPROXY_LOG("Error! Real opengl32.dll not loaded. Can't get function %s", funcName);
        return nullptr;
    }

    auto fptr = GetProcAddress(m_dllHandle, funcName);
    if (fptr == nullptr)
    {
        GLPROXY_LOG("Error! Unable to find %s", funcName);
    }

    return reinterpret_cast<void*>(fptr);
}

OpenGLDll& OpenGLDll::getInstance() // static
{
    // Just one instance per process.
    // Also only attempt to load the DLL on the first reference.
    static OpenGLDll s_TheGlDll;
    return s_TheGlDll;
}

void* OpenGLDll::getRealGLFunc(const char* funcName) // static
{
    auto& glDll = OpenGLDll::getInstance();
    void* addr = glDll.getFuncPtr(funcName);
    GLPROXY_LOG("Loading real GL func: (%s) %s", War3::ptrToString(addr).c_str(), funcName);
    return addr;
}

// ========================================================
// g_RealGLFunctions linked list:
// ========================================================

GLFuncBase* g_RealGLFunctions = nullptr;

// ========================================================
// AutoReport:
// ========================================================

#if GLPROXY_WITH_LOG
AutoReport::AutoReport()
{
    GLPROXY_LOG("\n--------------------------------------------------------");
    GLPROXY_LOG("  OPENGL32.DLL proxy report - %s", War3::getTimeString().c_str());
    GLPROXY_LOG("--------------------------------------------------------\n");
}

AutoReport::~AutoReport()
{
    writeReport();
}

void AutoReport::writeReport() // static
{
    // Gather all function pointers first so we can sort them by call count.
    std::vector<const GLFuncBase*> sortedFuncs;
    for (const GLFuncBase* func = g_RealGLFunctions; func != nullptr; func = func->next)
    {
        sortedFuncs.push_back(func);
    }

    // Higher call counts first. If same call count then sort alphabetically by name.
    std::sort(std::begin(sortedFuncs), std::end(sortedFuncs),
              [](const GLFuncBase* a, const GLFuncBase* b) -> bool
              {
                  if (a->callCount == b->callCount)
                  {
                      return std::strcmp(a->name, b->name) < 0;
                  }
                  else
                  {
                      return a->callCount > b->callCount;
                  }
              });

    GLPROXY_LOG("--------------------------------------------------------");
    GLPROXY_LOG("  Function call counts (war3.exe/game.dll only)");
    GLPROXY_LOG("--------------------------------------------------------\n");

    for (const GLFuncBase* func : sortedFuncs)
    {
        GLPROXY_LOG("%s %s", War3::numToString(func->callCount).c_str(), func->name);
    }

    GLPROXY_LOG("\n%zu GL functions were called by the application.", sortedFuncs.size());
    GLProxy::getProxyDllLogStream().flush();
}

// This static instance will open the GLProxy log on startup and
// write the function call counts on shutdown via its destructor.
AutoReport g_AutoReport;
#endif // GLPROXY_WITH_LOG

// ========================================================

} // namespace GLProxy
