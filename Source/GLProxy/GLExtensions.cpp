
// ============================================================================
// File:   GLExtensions.cpp
// Author: Guilherme R. Lampert
// Brief:  Pointers to extended functions in the real opengl32.dll
// ============================================================================

#include "GLProxy/GLExtensions.hpp"
#include "GLProxy/GLDllUtils.hpp"

namespace GLProxy
{

// ========================================================
// GL function pointers and extensions loading:
// ========================================================

#define GL_FUNC_PTR_VARDEF(funcName) WAR3_STRING_JOIN2(pfn_, funcName) funcName = nullptr;
    GLPROXY_FUNCTION_POINTERS_LIST(GL_FUNC_PTR_VARDEF)
#undef GL_FUNC_PTR_VARDEF

#define GET_GL_FUNC(funcName)                                                                                                            \
    do {                                                                                                                                 \
        GLProxy::funcName = reinterpret_cast<WAR3_STRING_JOIN2(pfn_, funcName)>(wglGetProcAddress(WAR3_STRINGIZE(funcName)));            \
        if (GLProxy::funcName == nullptr)                                                                                                \
        {                                                                                                                                \
            GLProxy::funcName = reinterpret_cast<WAR3_STRING_JOIN2(pfn_, funcName)>(OpenGLDll::getRealGLFunc(WAR3_STRINGIZE(funcName))); \
            if (GLProxy::funcName == nullptr)                                                                                            \
            {                                                                                                                            \
                GLPROXY_LOG("WARNING: Failed to load ext func '%s'!", WAR3_STRINGIZE(funcName));                                         \
            }                                                                                                                            \
        }                                                                                                                                \
    } while (0,0)

// ========================================================
// GLProxy::loadInternalGLFunctions()
// ========================================================

void loadInternalGLFunctions()
{
    static bool s_functionsLoaded = false;
    if (s_functionsLoaded)
    {
        return;
    }

    GLPROXY_LOG("\n**** Loading War3HD GL extensions and internal function pointers ****\n");

    #define GL_LOAD_FUNC_PTR(funcName) GET_GL_FUNC(funcName);
        GLPROXY_FUNCTION_POINTERS_LIST(GL_LOAD_FUNC_PTR)
    #undef GL_LOAD_FUNC_PTR

    GLPROXY_CHECK_GL_ERRORS();

    GLPROXY_LOG("\n**** loadInternalGLFunctions() - DONE ****\n");
    s_functionsLoaded = true;
}

#undef GET_GL_FUNC

// ========================================================
// GL error checking:
// ========================================================

void checkGLErrors(const char* function, const char* filename, const int lineNum, const bool crash)
{
    struct Err
    {
        static const char* toString(const GLenum errorCode) noexcept
        {
            switch (errorCode)
            {
            case GL_NO_ERROR                      : return "GL_NO_ERROR";
            case GL_INVALID_ENUM                  : return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE                 : return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION             : return "GL_INVALID_OPERATION";
            case GL_INVALID_FRAMEBUFFER_OPERATION : return "GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY                 : return "GL_OUT_OF_MEMORY";
            case GL_STACK_UNDERFLOW               : return "GL_STACK_UNDERFLOW"; // Legacy; not used on GL3+
            case GL_STACK_OVERFLOW                : return "GL_STACK_OVERFLOW";  // Legacy; not used on GL3+
            default                               : return "Unknown GL error";
            } // switch (errorCode)
        }
    };

    GLint errorCount = 0;
    GLenum errorCode = GLProxy::glGetError();
    char buffer[2048];

    while (errorCode != GL_NO_ERROR)
    {
        std::snprintf(buffer, sizeof(buffer), "OpenGL error %X ( %s ) in %s(), file %s(%d).",
                      errorCode, Err::toString(errorCode), function, filename, lineNum);

        errorCode = GLProxy::glGetError();
        errorCount++;

        GLPROXY_LOG("WARNING: %s", buffer);
    }

    if (errorCount > 0 && crash)
    {
        GLProxy::fatalError("%d OpenGL errors were detected! Aborting.", errorCount);
    }
}

// ========================================================
// GLProxy log:
// ========================================================

void fatalError(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    va_list vaArgs;
    char message[2048] = {'\0'};

    va_start(vaArgs, fmt);
    std::vsnprintf(message, sizeof(message), fmt, vaArgs);
    va_end(vaArgs);

    #if GLPROXY_WITH_LOG
    auto& log = GLProxy::getProxyDllLogStream();
    log.writeF("GLProxy fatal error: %s\n", message);
    log.flush();
    #endif // GLPROXY_WITH_LOG

    MessageBoxA(nullptr, message, "GLProxy Fatal Error", MB_OK | MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}

#if GLPROXY_WITH_LOG
War3::LogStream& getProxyDllLogStream()
{
    static War3::LogStream s_TheLog{ "GLProxy.log" };
    return s_TheLog;
}
#endif // GLPROXY_WITH_LOG

// ========================================================

} // namespace GLProxy
