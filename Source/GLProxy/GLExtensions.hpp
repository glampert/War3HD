
// ================================================================================================
// -*- C++ -*-
// File: GLExtensions.hpp
// Author: Guilherme R. Lampert
// Created on: 26/11/15
// Brief: Pointers to extended functions in the real opengl32.dll
// ================================================================================================

#ifndef GLPROXY_GL_EXTENSIONS_HPP
#define GLPROXY_GL_EXTENSIONS_HPP

// GL types and constants:
#include "GLProxy/GLEnums.hpp"

// Trim down the WinAPI crap. We also don't want WinGDI.h
// to interfere with our WGL wrapper declarations.
#define NOGDI
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Extended GL functions will be members of this namespace.
namespace GLProxy
{

// These are direct pointers to the actual OpenGL library:
extern pfn_glDisable                    glDisable;
extern pfn_glEnable                     glEnable;
extern pfn_glPushAttrib                 glPushAttrib;
extern pfn_glPopAttrib                  glPopAttrib;
extern pfn_glPushClientAttrib           glPushClientAttrib;
extern pfn_glPopClientAttrib            glPopClientAttrib;
extern pfn_glGetString                  glGetString;
extern pfn_glGetError                   glGetError;
extern pfn_glGetIntegerv                glGetIntegerv;
extern pfn_glViewport                   glViewport;
extern pfn_glReadBuffer                 glReadBuffer;
extern pfn_glReadPixels                 glReadPixels;
extern pfn_glCreateProgram              glCreateProgram;
extern pfn_glCreateShader               glCreateShader;
extern pfn_glAttachShader               glAttachShader;
extern pfn_glCompileShader              glCompileShader;
extern pfn_glDeleteProgram              glDeleteProgram;
extern pfn_glDeleteShader               glDeleteShader;
extern pfn_glDetachShader               glDetachShader;
extern pfn_glLinkProgram                glLinkProgram;
extern pfn_glShaderSource               glShaderSource;
extern pfn_glUseProgram                 glUseProgram;
extern pfn_glGetProgramInfoLog          glGetProgramInfoLog;
extern pfn_glGetShaderInfoLog           glGetShaderInfoLog;
extern pfn_glGetProgramiv               glGetProgramiv;
extern pfn_glGetShaderiv                glGetShaderiv;
extern pfn_glGetUniformLocation         glGetUniformLocation;
extern pfn_glUniform1f                  glUniform1f;
extern pfn_glUniform2f                  glUniform2f;
extern pfn_glUniform3f                  glUniform3f;
extern pfn_glUniform4f                  glUniform4f;
extern pfn_glUniform1i                  glUniform1i;
extern pfn_glUniform2i                  glUniform2i;
extern pfn_glUniform3i                  glUniform3i;
extern pfn_glUniform4i                  glUniform4i;
extern pfn_glUniformMatrix3fv           glUniformMatrix3fv;
extern pfn_glUniformMatrix4fv           glUniformMatrix4fv;
extern pfn_glBindTexture                glBindTexture;
extern pfn_glActiveTexture              glActiveTexture;
extern pfn_glGenTextures                glGenTextures;
extern pfn_glDeleteTextures             glDeleteTextures;
extern pfn_glTexStorage2D               glTexStorage2D;
extern pfn_glTexParameteri              glTexParameteri;
extern pfn_glTexParameterf              glTexParameterf;
extern pfn_glPixelStorei                glPixelStorei;
extern pfn_glGetTexImage                glGetTexImage;
extern pfn_glGenerateMipmap             glGenerateMipmap;
extern pfn_glIsFramebuffer              glIsFramebuffer;
extern pfn_glBindFramebuffer            glBindFramebuffer;
extern pfn_glDeleteFramebuffers         glDeleteFramebuffers;
extern pfn_glGenFramebuffers            glGenFramebuffers;
extern pfn_glCheckFramebufferStatus     glCheckFramebufferStatus;
extern pfn_glFramebufferTexture2D       glFramebufferTexture2D;
extern pfn_glBlitFramebuffer            glBlitFramebuffer;
extern pfn_glDrawArrays                 glDrawArrays;
extern pfn_glDrawElements               glDrawElements;
extern pfn_glEnableClientState          glEnableClientState;
extern pfn_glDisableClientState         glDisableClientState;
extern pfn_glVertexPointer              glVertexPointer;
extern pfn_glColorPointer               glColorPointer;
extern pfn_glTexCoordPointer            glTexCoordPointer;
extern pfn_glNormalPointer              glNormalPointer;

// Loads all the above function pointers from the real OpenGL DLL. They are null
// until this is called at least once! Calling this if already initialized is a no-op.
void initializeExtensions();

//
// glGetError helpers:
//
#define GLPROXY_CHECK_GL_ERRORS() GLProxy::checkGLErrors(__FUNCTION__, __FILE__, __LINE__, false)
void checkGLErrors(const char * function, const char * filename, int lineNum, bool crash);

} // namespace GLProxy {}

// Our wrapper for the real wglGetProcAddress.
// Also used to get pointers to the extended functions we inject into War3.
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetProcAddress(LPCSTR funcName);

#endif // GLPROXY_GL_EXTENSIONS_HPP
