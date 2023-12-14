
#pragma once

// ============================================================================
// File:   GLExtensions.hpp
// Author: Guilherme R. Lampert
// Brief:  Pointers to extended functions in the real opengl32.dll
// ============================================================================

#include "War3/Common.hpp"
#include "GLProxy/GLEnums.hpp"

// Trim down the WinAPI crap. We also don't want WinGDI.h
// to interfere with our WGL wrapper declarations.
#define NOGDI
#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Strips out the proxy DLL log when zero.
#define GLPROXY_WITH_LOG 1

// We are not including 'WinGDI.h' and 'gl.h', so the
// required types must be redefined in this source file.
#define GLPROXY_NEED_WGL_STRUCTS 1

namespace GLProxy
{

// ========================================================

// These are direct pointers to the actual OpenGL library
#define GLPROXY_FUNCTION_POINTERS_LIST(FUNC_PTR) \
    FUNC_PTR(glDisable)                          \
    FUNC_PTR(glEnable)                           \
    FUNC_PTR(glIsEnabled)                        \
    FUNC_PTR(glPushAttrib)                       \
    FUNC_PTR(glPopAttrib)                        \
    FUNC_PTR(glPushClientAttrib)                 \
    FUNC_PTR(glPopClientAttrib)                  \
    FUNC_PTR(glGetString)                        \
    FUNC_PTR(glGetError)                         \
    FUNC_PTR(glGetIntegerv)                      \
    FUNC_PTR(glViewport)                         \
    FUNC_PTR(glReadBuffer)                       \
    FUNC_PTR(glReadPixels)                       \
    FUNC_PTR(glCreateProgram)                    \
    FUNC_PTR(glCreateShader)                     \
    FUNC_PTR(glAttachShader)                     \
    FUNC_PTR(glCompileShader)                    \
    FUNC_PTR(glDeleteProgram)                    \
    FUNC_PTR(glDeleteShader)                     \
    FUNC_PTR(glDetachShader)                     \
    FUNC_PTR(glLinkProgram)                      \
    FUNC_PTR(glProgramParameteri)                \
    FUNC_PTR(glShaderSource)                     \
    FUNC_PTR(glUseProgram)                       \
    FUNC_PTR(glGetProgramInfoLog)                \
    FUNC_PTR(glGetShaderInfoLog)                 \
    FUNC_PTR(glGetProgramiv)                     \
    FUNC_PTR(glGetShaderiv)                      \
    FUNC_PTR(glGetUniformLocation)               \
    FUNC_PTR(glUniform1f)                        \
    FUNC_PTR(glUniform2f)                        \
    FUNC_PTR(glUniform3f)                        \
    FUNC_PTR(glUniform4f)                        \
    FUNC_PTR(glUniform1i)                        \
    FUNC_PTR(glUniform2i)                        \
    FUNC_PTR(glUniform3i)                        \
    FUNC_PTR(glUniform4i)                        \
    FUNC_PTR(glUniformMatrix3fv)                 \
    FUNC_PTR(glUniformMatrix4fv)                 \
    FUNC_PTR(glBindTexture)                      \
    FUNC_PTR(glActiveTexture)                    \
    FUNC_PTR(glGenTextures)                      \
    FUNC_PTR(glDeleteTextures)                   \
    FUNC_PTR(glTexStorage2D)                     \
    FUNC_PTR(glTexImage2D)                       \
    FUNC_PTR(glTexParameteri)                    \
    FUNC_PTR(glTexParameterf)                    \
    FUNC_PTR(glPixelStorei)                      \
    FUNC_PTR(glGetTexImage)                      \
    FUNC_PTR(glGenerateMipmap)                   \
    FUNC_PTR(glIsFramebuffer)                    \
    FUNC_PTR(glBindFramebuffer)                  \
    FUNC_PTR(glDeleteFramebuffers)               \
    FUNC_PTR(glGenFramebuffers)                  \
    FUNC_PTR(glCheckFramebufferStatus)           \
    FUNC_PTR(glFramebufferTexture2D)             \
    FUNC_PTR(glBlitFramebuffer)                  \
    FUNC_PTR(glDrawArrays)                       \
    FUNC_PTR(glDrawElements)                     \
    FUNC_PTR(glEnableClientState)                \
    FUNC_PTR(glDisableClientState)               \
    FUNC_PTR(glVertexPointer)                    \
    FUNC_PTR(glColorPointer)                     \
    FUNC_PTR(glTexCoordPointer)                  \
    FUNC_PTR(glNormalPointer)                    \
    FUNC_PTR(glBlendFunc)                        \
    FUNC_PTR(glPolygonMode)                      \
    FUNC_PTR(glScissor)                          \
    FUNC_PTR(glLoadIdentity)                     \
    FUNC_PTR(glMatrixMode)                       \
    FUNC_PTR(glPushMatrix)                       \
    FUNC_PTR(glPopMatrix)                        \
    FUNC_PTR(glOrtho)                            \
    FUNC_PTR(glBlendEquation)                    \
    FUNC_PTR(glBindVertexArray)                  \
    FUNC_PTR(glBindBuffer)                       \
    FUNC_PTR(glEnableVertexAttribArray)          \
    FUNC_PTR(glVertexAttribPointer)              \
    FUNC_PTR(glGenVertexArrays)                  \
    FUNC_PTR(glDeleteVertexArrays)               \
    FUNC_PTR(glBufferData)                       \
    FUNC_PTR(glBlendEquationSeparate)            \
    FUNC_PTR(glBlendFuncSeparate)                \
    FUNC_PTR(glGetAttribLocation)                \
    FUNC_PTR(glGenBuffers)                       \
    FUNC_PTR(glDeleteBuffers)                    \
    FUNC_PTR(glDrawElementsBaseVertex)           \
    FUNC_PTR(glClearColor)                       \
    FUNC_PTR(glClear)

#define GL_FUNC_PTR_EXTERN(funcName) extern WAR3_STRING_JOIN2(pfn_, funcName) funcName;
    GLPROXY_FUNCTION_POINTERS_LIST(GL_FUNC_PTR_EXTERN)
#undef GL_FUNC_PTR_EXTERN

// Loads all the above function pointers from the real OpenGL DLL. They are null
// until this is called at least once! Calling this if already initialized is a no-op.
void loadInternalGLFunctions();

// ========================================================

// glGetError helpers:
void checkGLErrors(const char* function, const char* filename, int lineNum, bool crash);
#define GLPROXY_CHECK_GL_ERRORS() GLProxy::checkGLErrors(__FUNCTION__, __FILE__, __LINE__, false)

// Proxy DLL fatal error reporter.
WAR3_ATTRIB_NORETURN
WAR3_ATTRIB_NOINLINE
void fatalError(WAR3_PRINTF_LIKE const char* fmt, ...);

// GL proxy DLL log wrapper (can be muted at compile-time)
#if GLPROXY_WITH_LOG
    War3::LogStream& getProxyDllLogStream();
    #define GLPROXY_LOG(fmt, ...)                        \
        do {                                             \
            auto& log = GLProxy::getProxyDllLogStream(); \
            log.writeF(fmt, __VA_ARGS__);                \
            log.write('\n');                             \
        } while (0,0)
#else // GLPROXY_WITH_LOG
    #define GLPROXY_LOG(fmt, ...) /* nothing */
#endif // GLPROXY_WITH_LOG

// ========================================================

} // namespace GLProxy

// Our wrapper for the real wglGetProcAddress.
// Also used to get pointers to the extended functions we inject into War3.
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetProcAddress(LPCSTR funcName);
