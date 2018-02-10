
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
// Extensions loading:
// ========================================================

pfn_glDisable                    glDisable                  = nullptr;
pfn_glEnable                     glEnable                   = nullptr;
pfn_glPushAttrib                 glPushAttrib               = nullptr;
pfn_glPopAttrib                  glPopAttrib                = nullptr;
pfn_glPushClientAttrib           glPushClientAttrib         = nullptr;
pfn_glPopClientAttrib            glPopClientAttrib          = nullptr;
pfn_glGetString                  glGetString                = nullptr;
pfn_glGetError                   glGetError                 = nullptr;
pfn_glGetIntegerv                glGetIntegerv              = nullptr;
pfn_glViewport                   glViewport                 = nullptr;
pfn_glReadBuffer                 glReadBuffer               = nullptr;
pfn_glReadPixels                 glReadPixels               = nullptr;
pfn_glCreateProgram              glCreateProgram            = nullptr;
pfn_glCreateShader               glCreateShader             = nullptr;
pfn_glAttachShader               glAttachShader             = nullptr;
pfn_glCompileShader              glCompileShader            = nullptr;
pfn_glDeleteProgram              glDeleteProgram            = nullptr;
pfn_glDeleteShader               glDeleteShader             = nullptr;
pfn_glDetachShader               glDetachShader             = nullptr;
pfn_glLinkProgram                glLinkProgram              = nullptr;
pfn_glShaderSource               glShaderSource             = nullptr;
pfn_glUseProgram                 glUseProgram               = nullptr;
pfn_glGetProgramInfoLog          glGetProgramInfoLog        = nullptr;
pfn_glGetShaderInfoLog           glGetShaderInfoLog         = nullptr;
pfn_glGetProgramiv               glGetProgramiv             = nullptr;
pfn_glGetShaderiv                glGetShaderiv              = nullptr;
pfn_glGetUniformLocation         glGetUniformLocation       = nullptr;
pfn_glUniform1f                  glUniform1f                = nullptr;
pfn_glUniform2f                  glUniform2f                = nullptr;
pfn_glUniform3f                  glUniform3f                = nullptr;
pfn_glUniform4f                  glUniform4f                = nullptr;
pfn_glUniform1i                  glUniform1i                = nullptr;
pfn_glUniform2i                  glUniform2i                = nullptr;
pfn_glUniform3i                  glUniform3i                = nullptr;
pfn_glUniform4i                  glUniform4i                = nullptr;
pfn_glUniformMatrix3fv           glUniformMatrix3fv         = nullptr;
pfn_glUniformMatrix4fv           glUniformMatrix4fv         = nullptr;
pfn_glBindTexture                glBindTexture              = nullptr;
pfn_glActiveTexture              glActiveTexture            = nullptr;
pfn_glGenTextures                glGenTextures              = nullptr;
pfn_glDeleteTextures             glDeleteTextures           = nullptr;
pfn_glTexStorage2D               glTexStorage2D             = nullptr;
pfn_glTexParameteri              glTexParameteri            = nullptr;
pfn_glTexParameterf              glTexParameterf            = nullptr;
pfn_glPixelStorei                glPixelStorei              = nullptr;
pfn_glGetTexImage                glGetTexImage              = nullptr;
pfn_glGenerateMipmap             glGenerateMipmap           = nullptr;
pfn_glIsFramebuffer              glIsFramebuffer            = nullptr;
pfn_glBindFramebuffer            glBindFramebuffer          = nullptr;
pfn_glDeleteFramebuffers         glDeleteFramebuffers       = nullptr;
pfn_glGenFramebuffers            glGenFramebuffers          = nullptr;
pfn_glCheckFramebufferStatus     glCheckFramebufferStatus   = nullptr;
pfn_glFramebufferTexture2D       glFramebufferTexture2D     = nullptr;
pfn_glBlitFramebuffer            glBlitFramebuffer          = nullptr;
pfn_glDrawArrays                 glDrawArrays               = nullptr;
pfn_glDrawElements               glDrawElements             = nullptr;
pfn_glEnableClientState          glEnableClientState        = nullptr;
pfn_glDisableClientState         glDisableClientState       = nullptr;
pfn_glVertexPointer              glVertexPointer            = nullptr;
pfn_glColorPointer               glColorPointer             = nullptr;
pfn_glTexCoordPointer            glTexCoordPointer          = nullptr;
pfn_glNormalPointer              glNormalPointer            = nullptr;
static bool                      g_ExtensionsLoaded         = false;

// ========================================================

#define GET_EXT_FUNC(funcName)                                                                                                           \
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

static void loadMiscFunctions()
{
    GET_EXT_FUNC(glDisable);
    GET_EXT_FUNC(glEnable);
    GET_EXT_FUNC(glPushAttrib);
    GET_EXT_FUNC(glPopAttrib);
    GET_EXT_FUNC(glPushClientAttrib);
    GET_EXT_FUNC(glPopClientAttrib);
    GET_EXT_FUNC(glGetString);
    GET_EXT_FUNC(glGetError);
    GET_EXT_FUNC(glGetIntegerv);
    GET_EXT_FUNC(glViewport);
    GET_EXT_FUNC(glReadBuffer);
    GET_EXT_FUNC(glReadPixels);
    GET_EXT_FUNC(glDrawArrays);
    GET_EXT_FUNC(glDrawElements);
    GET_EXT_FUNC(glEnableClientState);
    GET_EXT_FUNC(glDisableClientState);
    GET_EXT_FUNC(glVertexPointer);
    GET_EXT_FUNC(glColorPointer);
    GET_EXT_FUNC(glTexCoordPointer);
    GET_EXT_FUNC(glNormalPointer);
}

static void loadTextureFunctions()
{
    GET_EXT_FUNC(glBindTexture);
    GET_EXT_FUNC(glGenTextures);
    GET_EXT_FUNC(glDeleteTextures);
    GET_EXT_FUNC(glTexParameteri);
    GET_EXT_FUNC(glTexParameterf);
    GET_EXT_FUNC(glPixelStorei);
    GET_EXT_FUNC(glGetTexImage);
    GET_EXT_FUNC(glActiveTexture);
    GET_EXT_FUNC(glTexStorage2D);
    GET_EXT_FUNC(glGenerateMipmap);
}

static void loadShaderFunctions()
{
    GET_EXT_FUNC(glCreateProgram);
    GET_EXT_FUNC(glCreateShader);
    GET_EXT_FUNC(glAttachShader);
    GET_EXT_FUNC(glCompileShader);
    GET_EXT_FUNC(glDeleteProgram);
    GET_EXT_FUNC(glDeleteShader);
    GET_EXT_FUNC(glDetachShader);
    GET_EXT_FUNC(glLinkProgram);
    GET_EXT_FUNC(glShaderSource);
    GET_EXT_FUNC(glUseProgram);
    GET_EXT_FUNC(glGetProgramInfoLog);
    GET_EXT_FUNC(glGetShaderInfoLog);
    GET_EXT_FUNC(glGetProgramiv);
    GET_EXT_FUNC(glGetShaderiv);
    GET_EXT_FUNC(glGetUniformLocation);
    GET_EXT_FUNC(glUniform1f);
    GET_EXT_FUNC(glUniform2f);
    GET_EXT_FUNC(glUniform3f);
    GET_EXT_FUNC(glUniform4f);
    GET_EXT_FUNC(glUniform1i);
    GET_EXT_FUNC(glUniform2i);
    GET_EXT_FUNC(glUniform3i);
    GET_EXT_FUNC(glUniform4i);
    GET_EXT_FUNC(glUniformMatrix3fv);
    GET_EXT_FUNC(glUniformMatrix4fv);
}

static void loadFramebufferFunctions()
{
    GET_EXT_FUNC(glIsFramebuffer);
    GET_EXT_FUNC(glBindFramebuffer);
    GET_EXT_FUNC(glDeleteFramebuffers);
    GET_EXT_FUNC(glGenFramebuffers);
    GET_EXT_FUNC(glCheckFramebufferStatus);
    GET_EXT_FUNC(glFramebufferTexture2D);
    GET_EXT_FUNC(glBlitFramebuffer);
}

#undef GET_EXT_FUNC

// ========================================================
// GLProxy::initializeExtensions()
// ========================================================

void initializeExtensions()
{
    if (g_ExtensionsLoaded)
    {
        return;
    }

    GLPROXY_LOG("\n**** Loading War3HD GL extensions ****\n");

    loadMiscFunctions();
    loadTextureFunctions();
    loadShaderFunctions();
    loadFramebufferFunctions();
    GLPROXY_CHECK_GL_ERRORS();

    GLPROXY_LOG("\n**** DONE! ****\n");
    g_ExtensionsLoaded = true;
}

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
