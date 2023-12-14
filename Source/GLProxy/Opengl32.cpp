
// ============================================================================
// File:   Opengl32.cpp
// Author: Guilherme R. Lampert
// Brief:  GLProxy intercepts all calls to the real OpenGL library.
//         This file is the public DLL interface of our "fake" opengl32.dll.
// ============================================================================

#include "GLExtensions.hpp"
#include "GLDllUtils.hpp"

#include "War3/Renderer.hpp"
#include "War3/Window.hpp"
#include "War3/DebugUI.hpp"

// ========================================================
// DllMain:
//  NOTE: Threads are not supported.
//  Probably a non issue, since OpenGL is single-threaded.
// ========================================================

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD reasonForDllLoad, LPVOID /*reserved*/)
{
    switch (reasonForDllLoad)
    {
    case DLL_PROCESS_ATTACH :
        GLPROXY_LOG("\nDllMain: DLL_PROCESS_ATTACH (hInstDll=0x%p)\n", hInstDll);
        DisableThreadLibraryCalls(hInstDll); // Disables the THREAD_ATTACH/DETACH messages.
        break;

    case DLL_PROCESS_DETACH :
        GLPROXY_LOG("\nDllMain: DLL_PROCESS_DETACH (hInstDll=0x%p)\n", hInstDll);
        break;

    default :
        break;
    } // switch (reasonForDllLoad)

    return TRUE;
}

// ========================================================
// These macros simplify declaring our wrapper functions:
// ========================================================

//
// Functions with a return value:
//
#define GLFUNC_0_WRET(retType, funcName)                         \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(void)           \
    {                                                            \
        static GLProxy::TGLFunc<retType> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName);                           \
    }

#define GLFUNC_1_WRET(retType, funcName, t0, p0)                     \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0)              \
    {                                                                \
        static GLProxy::TGLFunc<retType, t0> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0);                           \
    }

#define GLFUNC_2_WRET(retType, funcName, t0, p0, t1, p1)                 \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1)           \
    {                                                                    \
        static GLProxy::TGLFunc<retType, t0, t1> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1);                           \
    }

#define GLFUNC_3_WRET(retType, funcName, t0, p0, t1, p1, t2, p2)             \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2)        \
    {                                                                        \
        static GLProxy::TGLFunc<retType, t0, t1, t2> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2);                           \
    }

#define GLFUNC_4_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3)         \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3)     \
    {                                                                            \
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3);                           \
    }

#define GLFUNC_5_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4)     \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4)  \
    {                                                                                \
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3, t4> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4);                           \
    }

#define GLFUNC_8_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7) \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
    {                                                                                                    \
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3, t4, t5, t6, t7> TGLFUNC_DECL(funcName);         \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7);                                   \
    }

//
// Functions returning void/nothing:
//
#define GLFUNC_0(funcName)                                    \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(void)           \
    {                                                         \
        static GLProxy::TGLFunc<void> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName);                               \
    }

#define GLFUNC_1(funcName, t0, p0)                                \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0)              \
    {                                                             \
        static GLProxy::TGLFunc<void, t0> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0);                               \
    }

#define GLFUNC_2(funcName, t0, p0, t1, p1)                            \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1)           \
    {                                                                 \
        static GLProxy::TGLFunc<void, t0, t1> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1);                               \
    }

#define GLFUNC_3(funcName, t0, p0, t1, p1, t2, p2)                        \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2)        \
    {                                                                     \
        static GLProxy::TGLFunc<void, t0, t1, t2> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2);                               \
    }

#define GLFUNC_4(funcName, t0, p0, t1, p1, t2, p2, t3, p3)                    \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3)     \
    {                                                                         \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3);                               \
    }

#define GLFUNC_5(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4)                \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4)  \
    {                                                                             \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4);                               \
    }

#define GLFUNC_6(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)              \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) \
    {                                                                                   \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5> TGLFUNC_DECL(funcName);   \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5);                                 \
    }

#define GLFUNC_7(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)             \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) \
    {                                                                                          \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6> TGLFUNC_DECL(funcName);      \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6);                                    \
    }

#define GLFUNC_8(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)            \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
    {                                                                                                 \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7> TGLFUNC_DECL(funcName);         \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7);                                       \
    }

#define GLFUNC_9(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)           \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8) \
    {                                                                                                        \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7, t8> TGLFUNC_DECL(funcName);            \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7, p8);                                          \
    }

#define GLFUNC_10(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)         \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9) \
    {                                                                                                               \
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9> TGLFUNC_DECL(funcName);               \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);                                             \
    }

// ========================================================
// "Classic OpenGL" and WGL wrappers:
// ========================================================

//
// WGL structures.
//
// They are defined here because including WinGDI.h would produce
// warnings or even errors about our redefined WGL function prototypes.
//
#if GLPROXY_NEED_WGL_STRUCTS
struct PIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
};
struct LAYERPLANEDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerPlane;
    BYTE  bReserved;
    COLORREF crTransparent;
};
struct GLYPHMETRICSFLOAT
{
    float gmfBlackBoxX;
    float gmfBlackBoxY;
    struct
    {
        float x;
        float y;
    } gmfptGlyphOrigin;
    float gmfCellIncX;
    float gmfCellIncY;
};
struct WGLSWAP
{
    HDC  hdc;
    UINT flags;
};
#endif // GLPROXY_NEED_WGL_STRUCTS

// ========================================================

static bool g_RendererIsStarted = false;

GLPROXY_EXTERN HGLRC GLPROXY_DECL wglCreateContext(HDC hdc)
{
    static GLProxy::TGLFunc<HGLRC, HDC> TGLFUNC_DECL(wglCreateContext);
    return TGLFUNC_CALL(wglCreateContext, hdc);
}

GLPROXY_EXTERN BOOL GLPROXY_DECL wglDeleteContext(HGLRC hglrc)
{
    static GLProxy::TGLFunc<BOOL, HGLRC> TGLFUNC_DECL(wglDeleteContext);

    // Application shutdown or window minimized
    War3::Renderer::getInstance().stop();
    g_RendererIsStarted = false;

    return TGLFUNC_CALL(wglDeleteContext, hglrc);
}

GLPROXY_EXTERN void GLPROXY_DECL glClear(GLbitfield mask)
{
    static GLProxy::TGLFunc<void, GLbitfield> TGLFUNC_DECL(glClear);

    // Can't do this from wglCreateContext for some reason...
    if (!g_RendererIsStarted)
    {
        War3::Renderer::getInstance().start();
        g_RendererIsStarted = true;
    }

    // glClear is probably a good place to start an intercepted render frame.
    War3::Renderer::getInstance().beginFrame();

    TGLFUNC_CALL(glClear, mask);
}

GLPROXY_EXTERN BOOL GLPROXY_DECL wglSwapLayerBuffers(HDC hdc, UINT flags)
{
    static GLProxy::TGLFunc<BOOL, HDC, UINT> TGLFUNC_DECL(wglSwapLayerBuffers);

    // War3 only uses SwapLayerBuffers to present the rendered frame.
    War3::Renderer::getInstance().endFrame();

    return TGLFUNC_CALL(wglSwapLayerBuffers, hdc, flags);
}

// wglGetProcAddress is a special case. We also want to log
// which extensions got dynamically loaded by the application.
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetProcAddress(LPCSTR funcName)
{
    static GLProxy::TGLFunc<PROC, LPCSTR> TGLFUNC_DECL(wglGetProcAddress);
    GLPROXY_LOG("wglGetProcAddress('%s')", funcName);
    return TGLFUNC_CALL(wglGetProcAddress, funcName);
}

// This is an undocumented function, it seems. So it is probably not called by most applications...
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetDefaultProcAddress(LPCSTR funcName)
{
    static GLProxy::TGLFunc<PROC, LPCSTR> TGLFUNC_DECL(wglGetDefaultProcAddress);
    GLPROXY_LOG("wglGetDefaultProcAddress('%s')", funcName);
    return TGLFUNC_CALL(wglGetDefaultProcAddress, funcName);
}

//
// WGL functions:
//
GLFUNC_0_WRET(HDC, wglGetCurrentDC);
GLFUNC_0_WRET(HGLRC, wglGetCurrentContext);
GLFUNC_1_WRET(BOOL, wglSwapBuffers, HDC, hdc);
GLFUNC_1_WRET(int, wglGetPixelFormat, HDC, hdc);
GLFUNC_2_WRET(BOOL, wglMakeCurrent, HDC, hdc, HGLRC, hglrc);
GLFUNC_2_WRET(BOOL, wglShareLists, HGLRC, hglrc1, HGLRC, hglrc2);
GLFUNC_2_WRET(DWORD, wglSwapMultipleBuffers, UINT, n, const WGLSWAP*, sw);
GLFUNC_2_WRET(HGLRC, wglCreateLayerContext, HDC, hdc, int, b);
GLFUNC_2_WRET(int, wglChoosePixelFormat, HDC, hdc, const PIXELFORMATDESCRIPTOR*, pfd);
GLFUNC_3_WRET(BOOL, wglCopyContext, HGLRC, hglrc1, HGLRC, hglrc2, UINT, flags);
GLFUNC_3_WRET(BOOL, wglRealizeLayerPalette, HDC, hdc, int, b, BOOL, c);
GLFUNC_3_WRET(BOOL, wglSetPixelFormat, HDC, hdc, int, b, const PIXELFORMATDESCRIPTOR*, pfd);
GLFUNC_4_WRET(BOOL, wglUseFontBitmapsA, HDC, hdc, DWORD, b, DWORD, c, DWORD, d);
GLFUNC_4_WRET(BOOL, wglUseFontBitmapsW, HDC, hdc, DWORD, b, DWORD, c, DWORD, d);
GLFUNC_4_WRET(int, wglDescribePixelFormat, HDC, hdc, int, b, UINT, c, PIXELFORMATDESCRIPTOR*, pfd);
GLFUNC_5_WRET(BOOL, wglDescribeLayerPlane, HDC, hdc, int, b, int, c, UINT, d, LAYERPLANEDESCRIPTOR*, lpd);
GLFUNC_5_WRET(int, wglGetLayerPaletteEntries, HDC, hdc, int, b, int, c, int, d, COLORREF*, e);
GLFUNC_5_WRET(int, wglSetLayerPaletteEntries, HDC, hdc, int, b, int, c, int, d, const COLORREF*, e);
GLFUNC_8_WRET(BOOL, wglUseFontOutlinesA, HDC, hdc, DWORD, b, DWORD, c, DWORD, d, float, e, float, f, int, g, GLYPHMETRICSFLOAT*, gmf);
GLFUNC_8_WRET(BOOL, wglUseFontOutlinesW, HDC, hdc, DWORD, b, DWORD, c, DWORD, d, float, e, float, f, int, g, GLYPHMETRICSFLOAT*, gmf);

//
// GL Functions with a return value:
//
GLFUNC_0_WRET(GLenum, glGetError);
GLFUNC_1_WRET(GLboolean, glIsEnabled, GLenum, cap);
GLFUNC_1_WRET(GLboolean, glIsList, GLuint, list);
GLFUNC_1_WRET(GLboolean, glIsTexture, GLuint, texture);
GLFUNC_1_WRET(GLint, glRenderMode, GLenum, mode);
GLFUNC_1_WRET(GLuint, glGenLists, GLsizei, range);
GLFUNC_1_WRET(const GLubyte*, glGetString, GLenum, name);
GLFUNC_3_WRET(GLboolean, glAreTexturesResident, GLsizei, n, const GLuint*, textures, GLboolean*, residences);

//
// GL Functions returning void:
//
GLFUNC_0(glEnd);
GLFUNC_0(glEndList);
GLFUNC_0(glFinish);
GLFUNC_0(glFlush);
GLFUNC_0(glInitNames);
GLFUNC_0(glLoadIdentity);
GLFUNC_0(glPopAttrib);
GLFUNC_0(glPopClientAttrib);
GLFUNC_0(glPopMatrix);
GLFUNC_0(glPopName);
GLFUNC_0(glPushMatrix)
GLFUNC_1(glArrayElement, GLint, i);
GLFUNC_1(glBegin, GLenum, mode);
GLFUNC_1(glCallList, GLuint, list);
GLFUNC_1(glClearDepth, GLclampd, depth);
GLFUNC_1(glClearIndex, GLfloat, c);
GLFUNC_1(glClearStencil, GLint, s);
GLFUNC_1(glColor3bv, const GLbyte*, v);
GLFUNC_1(glColor3dv, const GLdouble*, v);
GLFUNC_1(glColor3fv, const GLfloat*, v);
GLFUNC_1(glColor3iv, const GLint*, v);
GLFUNC_1(glColor3sv, const GLshort*, v);
GLFUNC_1(glColor3ubv, const GLubyte*, v);
GLFUNC_1(glColor3uiv, const GLuint*, v);
GLFUNC_1(glColor3usv, const GLushort*, v);
GLFUNC_1(glColor4bv, const GLbyte*, v);
GLFUNC_1(glColor4dv, const GLdouble*, v);
GLFUNC_1(glColor4fv, const GLfloat*, v);
GLFUNC_1(glColor4iv, const GLint*, v);
GLFUNC_1(glColor4sv, const GLshort*, v);
GLFUNC_1(glColor4ubv, const GLubyte*, v);
GLFUNC_1(glColor4uiv, const GLuint*, v);
GLFUNC_1(glColor4usv, const GLushort*, v);
GLFUNC_1(glCullFace, GLenum, mode);
GLFUNC_1(glDepthFunc, GLenum, func);
GLFUNC_1(glDepthMask, GLboolean, flag);
GLFUNC_1(glDisable, GLenum, cap);
GLFUNC_1(glDisableClientState, GLenum, array);
GLFUNC_1(glDrawBuffer, GLenum, mode);
GLFUNC_1(glEdgeFlag, GLboolean, flag);
GLFUNC_1(glEdgeFlagv, const GLboolean*, flag);
GLFUNC_1(glEnable, GLenum, cap);
GLFUNC_1(glEnableClientState, GLenum, array);
GLFUNC_1(glEvalCoord1d, GLdouble, u);
GLFUNC_1(glEvalCoord1dv, const GLdouble*, u);
GLFUNC_1(glEvalCoord1f, GLfloat, u);
GLFUNC_1(glEvalCoord1fv, const GLfloat*, u);
GLFUNC_1(glEvalCoord2dv, const GLdouble*, u);
GLFUNC_1(glEvalCoord2fv, const GLfloat*, u);
GLFUNC_1(glEvalPoint1, GLint, i);
GLFUNC_1(glFrontFace, GLenum, mode);
GLFUNC_1(glGetPolygonStipple, GLubyte*, mask);
GLFUNC_1(glIndexMask, GLuint, mask);
GLFUNC_1(glIndexd, GLdouble, c);
GLFUNC_1(glIndexdv, const GLdouble*, c);
GLFUNC_1(glIndexf, GLfloat, c);
GLFUNC_1(glIndexfv, const GLfloat*, c);
GLFUNC_1(glIndexi, GLint, c);
GLFUNC_1(glIndexiv, const GLint*, c);
GLFUNC_1(glIndexs, GLshort, c);
GLFUNC_1(glIndexsv, const GLshort*, c);
GLFUNC_1(glIndexub, GLubyte, c);
GLFUNC_1(glIndexubv, const GLubyte*, c);
GLFUNC_1(glLineWidth, GLfloat, width);
GLFUNC_1(glListBase, GLuint, base);
GLFUNC_1(glLoadMatrixd, const GLdouble*, m);
GLFUNC_1(glLoadMatrixf, const GLfloat*, m);
GLFUNC_1(glLoadName, GLuint, name);
GLFUNC_1(glLogicOp, GLenum, opcode);
GLFUNC_1(glMatrixMode, GLenum, mode);
GLFUNC_1(glMultMatrixd, const GLdouble*, m);
GLFUNC_1(glMultMatrixf, const GLfloat*, m);
GLFUNC_1(glNormal3bv, const GLbyte*, v);
GLFUNC_1(glNormal3dv, const GLdouble*, v);
GLFUNC_1(glNormal3fv, const GLfloat*, v);
GLFUNC_1(glNormal3iv, const GLint*, v);
GLFUNC_1(glNormal3sv, const GLshort*, v);
GLFUNC_1(glPassThrough, GLfloat, token);
GLFUNC_1(glPointSize, GLfloat, size);
GLFUNC_1(glPolygonStipple, const GLubyte*, mask);
GLFUNC_1(glPushAttrib, GLbitfield, mask);
GLFUNC_1(glPushClientAttrib, GLbitfield, mask);
GLFUNC_1(glPushName, GLuint, name);
GLFUNC_1(glRasterPos2dv, const GLdouble*, v);
GLFUNC_1(glRasterPos2fv, const GLfloat*, v);
GLFUNC_1(glRasterPos2iv, const GLint*, v);
GLFUNC_1(glRasterPos2sv, const GLshort*, v);
GLFUNC_1(glRasterPos3dv, const GLdouble*, v);
GLFUNC_1(glRasterPos3fv, const GLfloat*, v);
GLFUNC_1(glRasterPos3iv, const GLint*, v);
GLFUNC_1(glRasterPos3sv, const GLshort*, v);
GLFUNC_1(glRasterPos4dv, const GLdouble*, v);
GLFUNC_1(glRasterPos4fv, const GLfloat*, v);
GLFUNC_1(glRasterPos4iv, const GLint*, v);
GLFUNC_1(glRasterPos4sv, const GLshort*, v);
GLFUNC_1(glReadBuffer, GLenum, mode);
GLFUNC_1(glShadeModel, GLenum, mode);
GLFUNC_1(glStencilMask, GLuint, mask);
GLFUNC_1(glTexCoord1d, GLdouble, s);
GLFUNC_1(glTexCoord1dv, const GLdouble*, v);
GLFUNC_1(glTexCoord1f, GLfloat, s);
GLFUNC_1(glTexCoord1fv, const GLfloat*, v);
GLFUNC_1(glTexCoord1i, GLint, s);
GLFUNC_1(glTexCoord1iv, const GLint*, v);
GLFUNC_1(glTexCoord1s, GLshort, s);
GLFUNC_1(glTexCoord1sv, const GLshort*, v);
GLFUNC_1(glTexCoord2dv, const GLdouble*, v);
GLFUNC_1(glTexCoord2fv, const GLfloat*, v);
GLFUNC_1(glTexCoord2iv, const GLint*, v);
GLFUNC_1(glTexCoord2sv, const GLshort*, v);
GLFUNC_1(glTexCoord3dv, const GLdouble*, v);
GLFUNC_1(glTexCoord3fv, const GLfloat*, v);
GLFUNC_1(glTexCoord3iv, const GLint*, v);
GLFUNC_1(glTexCoord3sv, const GLshort*, v);
GLFUNC_1(glTexCoord4dv, const GLdouble*, v);
GLFUNC_1(glTexCoord4fv, const GLfloat*, v);
GLFUNC_1(glTexCoord4iv, const GLint*, v);
GLFUNC_1(glTexCoord4sv, const GLshort*, v);
GLFUNC_1(glVertex2dv, const GLdouble*, v);
GLFUNC_1(glVertex2fv, const GLfloat*, v);
GLFUNC_1(glVertex2iv, const GLint*, v);
GLFUNC_1(glVertex2sv, const GLshort*, v);
GLFUNC_1(glVertex3dv, const GLdouble*, v);
GLFUNC_1(glVertex3fv, const GLfloat*, v);
GLFUNC_1(glVertex3iv, const GLint*, v);
GLFUNC_1(glVertex3sv, const GLshort*, v);
GLFUNC_1(glVertex4dv, const GLdouble*, v);
GLFUNC_1(glVertex4fv, const GLfloat*, v);
GLFUNC_1(glVertex4iv, const GLint*, v);
GLFUNC_1(glVertex4sv, const GLshort*, v);
GLFUNC_2(glAccum, GLenum, op, GLfloat, value);
GLFUNC_2(glAlphaFunc, GLenum, func, GLclampf, ref);
GLFUNC_2(glBindTexture, GLenum, target, GLuint, texture);
GLFUNC_2(glBlendFunc, GLenum, sfactor, GLenum, dfactor);
GLFUNC_2(glClipPlane, GLenum, plane, const GLdouble*, equation);
GLFUNC_3(glColor3b, GLbyte, red, GLbyte, green, GLbyte, blue);
GLFUNC_2(glColorMaterial, GLenum, face, GLenum, mode);
GLFUNC_2(glDeleteLists, GLuint, list, GLsizei, range);
GLFUNC_2(glDeleteTextures, GLsizei, n, const GLuint*, textures);
GLFUNC_2(glDepthRange, GLclampd, zNear, GLclampd, zFar);
GLFUNC_2(glEdgeFlagPointer, GLsizei, stride, const void*, pointer);
GLFUNC_2(glEvalCoord2d, GLdouble, u, GLdouble, v);
GLFUNC_2(glEvalCoord2f, GLfloat, u, GLfloat, v);
GLFUNC_2(glEvalPoint2, GLint, i, GLint, j);
GLFUNC_2(glFogf, GLenum, pname, GLfloat, param);
GLFUNC_2(glFogfv, GLenum, pname, const GLfloat*, params);
GLFUNC_2(glFogi, GLenum, pname, GLint, param);
GLFUNC_2(glFogiv, GLenum, pname, const GLint*, params);
GLFUNC_2(glGenTextures, GLsizei, n, GLuint*, textures);
GLFUNC_2(glGetBooleanv, GLenum, pname, GLboolean*, params);
GLFUNC_2(glGetClipPlane, GLenum, plane, GLdouble*, equation);
GLFUNC_2(glGetDoublev, GLenum, pname, GLdouble*, params);
GLFUNC_2(glGetFloatv, GLenum, pname, GLfloat*, params);
GLFUNC_2(glGetIntegerv, GLenum, pname, GLint*, params);
GLFUNC_2(glGetPixelMapfv, GLenum, map, GLfloat*, values);
GLFUNC_2(glGetPixelMapuiv, GLenum, map, GLuint*, values);
GLFUNC_2(glGetPixelMapusv, GLenum, map, GLushort*, values);
GLFUNC_2(glGetPointerv, GLenum, pname, void**, params);
GLFUNC_2(glHint, GLenum, target, GLenum, mode);
GLFUNC_2(glLightModelf, GLenum, pname, GLfloat, param);
GLFUNC_2(glLightModelfv, GLenum, pname, const GLfloat*, params);
GLFUNC_2(glLightModeli, GLenum, pname, GLint, param);
GLFUNC_2(glLightModeliv, GLenum, pname, const GLint*, params);
GLFUNC_2(glLineStipple, GLint, factor, GLushort, pattern);
GLFUNC_2(glNewList, GLuint, list, GLenum, mode);
GLFUNC_2(glPixelStoref, GLenum, pname, GLfloat, param);
GLFUNC_2(glPixelStorei, GLenum, pname, GLint, param);
GLFUNC_2(glPixelTransferf, GLenum, pname, GLfloat, param);
GLFUNC_2(glPixelTransferi, GLenum, pname, GLint, param);
GLFUNC_2(glPixelZoom, GLfloat, xfactor, GLfloat, yfactor);
GLFUNC_2(glPolygonMode, GLenum, face, GLenum, mode);
GLFUNC_2(glPolygonOffset, GLfloat, factor, GLfloat, units);
GLFUNC_2(glRasterPos2d, GLdouble, x, GLdouble, y);
GLFUNC_2(glRasterPos2f, GLfloat, x, GLfloat, y);
GLFUNC_2(glRasterPos2i, GLint, x, GLint, y);
GLFUNC_2(glRasterPos2s, GLshort, x, GLshort, y);
GLFUNC_3(glRasterPos3i, GLint, x, GLint, y, GLint, z);
GLFUNC_2(glRectdv, const GLdouble*, v1, const GLdouble*, v2);
GLFUNC_2(glRectfv, const GLfloat*, v1, const GLfloat*, v2);
GLFUNC_2(glRectiv, const GLint*, v1, const GLint*, v2);
GLFUNC_2(glRectsv, const GLshort*, v1, const GLshort*, v2);
GLFUNC_2(glSelectBuffer, GLsizei, size, GLuint*, buffer);
GLFUNC_2(glTexCoord2d, GLdouble, s, GLdouble, t);
GLFUNC_2(glTexCoord2f, GLfloat, s, GLfloat, t);
GLFUNC_2(glTexCoord2i, GLint, s, GLint, t);
GLFUNC_2(glTexCoord2s, GLshort, s, GLshort, t);
GLFUNC_2(glVertex2d, GLdouble, x, GLdouble, y);
GLFUNC_2(glVertex2f, GLfloat, x, GLfloat, y);
GLFUNC_2(glVertex2i, GLint, x, GLint, y);
GLFUNC_2(glVertex2s, GLshort, x, GLshort, y);
GLFUNC_3(glCallLists, GLsizei, n, GLenum, type, const void*, lists);
GLFUNC_3(glColor3d, GLdouble, red, GLdouble, green, GLdouble, blue);
GLFUNC_3(glColor3f, GLfloat, red, GLfloat, green, GLfloat, blue);
GLFUNC_3(glColor3i, GLint, red, GLint, green, GLint, blue);
GLFUNC_3(glColor3s, GLshort, red, GLshort, green, GLshort, blue);
GLFUNC_3(glColor3ub, GLubyte, red, GLubyte, green, GLubyte, blue);
GLFUNC_3(glColor3ui, GLuint, red, GLuint, green, GLuint, blue);
GLFUNC_3(glColor3us, GLushort, red, GLushort, green, GLushort, blue);
GLFUNC_3(glDrawArrays, GLenum, mode, GLint, first, GLsizei, count);
GLFUNC_3(glEvalMesh1, GLenum, mode, GLint, i1, GLint, i2);
GLFUNC_3(glFeedbackBuffer, GLsizei, size, GLenum, type, GLfloat*, buffer);
GLFUNC_3(glGetLightfv, GLenum, light, GLenum, pname, GLfloat*, params);
GLFUNC_3(glGetLightiv, GLenum, light, GLenum, pname, GLint*, params);
GLFUNC_3(glGetMapdv, GLenum, target, GLenum, query, GLdouble*, v);
GLFUNC_3(glGetMapfv, GLenum, target, GLenum, query, GLfloat*, v);
GLFUNC_3(glGetMapiv, GLenum, target, GLenum, query, GLint*, v);
GLFUNC_3(glGetMaterialfv, GLenum, face, GLenum, pname, GLfloat*, params);
GLFUNC_3(glGetMaterialiv, GLenum, face, GLenum, pname, GLint*, params);
GLFUNC_3(glGetTexEnvfv, GLenum, target, GLenum, pname, GLfloat*, params);
GLFUNC_3(glGetTexEnviv, GLenum, target, GLenum, pname, GLint*, params);
GLFUNC_3(glGetTexGendv, GLenum, coord, GLenum, pname, GLdouble*, params);
GLFUNC_3(glGetTexGenfv, GLenum, coord, GLenum, pname, GLfloat*, params);
GLFUNC_3(glGetTexGeniv, GLenum, coord, GLenum, pname, GLint*, params);
GLFUNC_3(glGetTexParameterfv, GLenum, target, GLenum, pname, GLfloat*, params);
GLFUNC_3(glGetTexParameteriv, GLenum, target, GLenum, pname, GLint*, params);
GLFUNC_3(glIndexPointer, GLenum, type, GLsizei, stride, const void*, pointer);
GLFUNC_3(glInterleavedArrays, GLenum, format, GLsizei, stride, const void*, pointer);
GLFUNC_3(glLightf, GLenum, light, GLenum, pname, GLfloat, param);
GLFUNC_3(glLightfv, GLenum, light, GLenum, pname, const GLfloat*, params);
GLFUNC_3(glLighti, GLenum, light, GLenum, pname, GLint, param);
GLFUNC_3(glLightiv, GLenum, light, GLenum, pname, const GLint*, params);
GLFUNC_3(glMapGrid1d, GLint, un, GLdouble, u1, GLdouble, u2);
GLFUNC_3(glMapGrid1f, GLint, un, GLfloat, u1, GLfloat, u2);
GLFUNC_3(glMaterialf, GLenum, face, GLenum, pname, GLfloat, param);
GLFUNC_3(glMaterialfv, GLenum, face, GLenum, pname, const GLfloat*, params);
GLFUNC_3(glMateriali, GLenum, face, GLenum, pname, GLint, param);
GLFUNC_3(glMaterialiv, GLenum, face, GLenum, pname, const GLint*, params);
GLFUNC_3(glNormal3b, GLbyte, nx, GLbyte, ny, GLbyte, nz);
GLFUNC_3(glNormal3d, GLdouble, nx, GLdouble, ny, GLdouble, nz);
GLFUNC_3(glNormal3f, GLfloat, nx, GLfloat, ny, GLfloat, nz);
GLFUNC_3(glNormal3i, GLint, nx, GLint, ny, GLint, nz);
GLFUNC_3(glNormal3s, GLshort, nx, GLshort, ny, GLshort, nz);
GLFUNC_3(glNormalPointer, GLenum, type, GLsizei, stride, const void*, pointer);
GLFUNC_3(glPixelMapfv, GLenum, map, GLsizei, mapsize, const GLfloat*, values);
GLFUNC_3(glPixelMapuiv, GLenum, map, GLsizei, mapsize, const GLuint*, values);
GLFUNC_3(glPixelMapusv, GLenum, map, GLsizei, mapsize, const GLushort*, values);
GLFUNC_3(glPrioritizeTextures, GLsizei, n, const GLuint*, textures, const GLclampf*, priorities);
GLFUNC_3(glRasterPos3d, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glRasterPos3f, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glRasterPos3s, GLshort, x, GLshort, y, GLshort, z);
GLFUNC_4(glRasterPos4d, GLdouble, x, GLdouble, y, GLdouble, z, GLdouble, w);
GLFUNC_3(glScaled, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glScalef, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glStencilFunc, GLenum, func, GLint, ref, GLuint, mask);
GLFUNC_3(glStencilOp, GLenum, fail, GLenum, zfail, GLenum, zpass);
GLFUNC_3(glTexCoord3d, GLdouble, s, GLdouble, t, GLdouble, r);
GLFUNC_3(glTexCoord3f, GLfloat, s, GLfloat, t, GLfloat, r);
GLFUNC_3(glTexCoord3i, GLint, s, GLint, t, GLint, r);
GLFUNC_3(glTexCoord3s, GLshort, s, GLshort, t, GLshort, r);
GLFUNC_3(glTexEnvf, GLenum, target, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexEnvfv, GLenum, target, GLenum, pname, const GLfloat*, params);
GLFUNC_3(glTexEnvi, GLenum, target, GLenum, pname, GLint, param);
GLFUNC_3(glTexEnviv, GLenum, target, GLenum, pname, const GLint*, params);
GLFUNC_3(glTexGend, GLenum, coord, GLenum, pname, GLdouble, param);
GLFUNC_3(glTexGendv, GLenum, coord, GLenum, pname, const GLdouble*, params);
GLFUNC_3(glTexGenf, GLenum, coord, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexGenfv, GLenum, coord, GLenum, pname, const GLfloat*, params);
GLFUNC_3(glTexGeni, GLenum, coord, GLenum, pname, GLint, param);
GLFUNC_3(glTexGeniv, GLenum, coord, GLenum, pname, const GLint*, params);
GLFUNC_3(glTexParameterf, GLenum, target, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexParameterfv, GLenum, target, GLenum, pname, const GLfloat*, params);
GLFUNC_3(glTexParameteri, GLenum, target, GLenum, pname, GLint, param);
GLFUNC_3(glTexParameteriv, GLenum, target, GLenum, pname, const GLint*, params);
GLFUNC_3(glTranslated, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glTranslatef, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glVertex3d, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glVertex3f, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glVertex3i, GLint, x, GLint, y, GLint, z);
GLFUNC_3(glVertex3s, GLshort, x, GLshort, y, GLshort, z);
GLFUNC_4(glClearAccum, GLfloat, red, GLfloat, green, GLfloat, blue, GLfloat, alpha);
GLFUNC_4(glClearColor, GLclampf, red, GLclampf, green, GLclampf, blue, GLclampf, alpha);
GLFUNC_4(glColor4b, GLbyte, red, GLbyte, green, GLbyte, blue, GLbyte, alpha);
GLFUNC_4(glColor4d, GLdouble, red, GLdouble, green, GLdouble, blue, GLdouble, alpha);
GLFUNC_4(glColor4f, GLfloat, red, GLfloat, green, GLfloat, blue, GLfloat, alpha);
GLFUNC_4(glColor4i, GLint, red, GLint, green, GLint, blue, GLint, alpha);
GLFUNC_4(glColor4s, GLshort, red, GLshort, green, GLshort, blue, GLshort, alpha);
GLFUNC_4(glColor4ub, GLubyte, red, GLubyte, green, GLubyte, blue, GLubyte, alpha);
GLFUNC_4(glColor4ui, GLuint, red, GLuint, green, GLuint, blue, GLuint, alpha);
GLFUNC_4(glColor4us, GLushort, red, GLushort, green, GLushort, blue, GLushort, alpha);
GLFUNC_4(glColorMask, GLboolean, red, GLboolean, green, GLboolean, blue, GLboolean, alpha);
GLFUNC_4(glColorPointer, GLint, size, GLenum, type, GLsizei, stride, const void*, pointer);
GLFUNC_4(glDrawElements, GLenum, mode, GLsizei, count, GLenum, type, const void*, indices);
GLFUNC_4(glGetTexLevelParameterfv, GLenum, target, GLint, level, GLenum, pname, GLfloat*, params);
GLFUNC_4(glGetTexLevelParameteriv, GLenum, target, GLint, level, GLenum, pname, GLint*, params);
GLFUNC_4(glRasterPos4f, GLfloat, x, GLfloat, y, GLfloat, z, GLfloat, w);
GLFUNC_4(glRasterPos4i, GLint, x, GLint, y, GLint, z, GLint, w);
GLFUNC_4(glRasterPos4s, GLshort, x, GLshort, y, GLshort, z, GLshort, w);
GLFUNC_4(glRectd, GLdouble, x1, GLdouble, y1, GLdouble, x2, GLdouble, y2);
GLFUNC_4(glRectf, GLfloat, x1, GLfloat, y1, GLfloat, x2, GLfloat, y2);
GLFUNC_4(glRecti, GLint, x1, GLint, y1, GLint, x2, GLint, y2);
GLFUNC_4(glRects, GLshort, x1, GLshort, y1, GLshort, x2, GLshort, y2);
GLFUNC_4(glRotated, GLdouble, angle, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_4(glRotatef, GLfloat, angle, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_4(glScissor, GLint, x, GLint, y, GLsizei, width, GLsizei, height);
GLFUNC_4(glTexCoord4d, GLdouble, s, GLdouble, t, GLdouble, r, GLdouble, q);
GLFUNC_4(glTexCoord4f, GLfloat, s, GLfloat, t, GLfloat, r, GLfloat, q);
GLFUNC_4(glTexCoord4i, GLint, s, GLint, t, GLint, r, GLint, q);
GLFUNC_4(glTexCoord4s, GLshort, s, GLshort, t, GLshort, r, GLshort, q);
GLFUNC_4(glTexCoordPointer, GLint, size, GLenum, type, GLsizei, stride, const void*, pointer);
GLFUNC_4(glVertex4d, GLdouble, x, GLdouble, y, GLdouble, z, GLdouble, w);
GLFUNC_4(glVertex4f, GLfloat, x, GLfloat, y, GLfloat, z, GLfloat, w);
GLFUNC_4(glVertex4i, GLint, x, GLint, y, GLint, z, GLint, w);
GLFUNC_4(glVertex4s, GLshort, x, GLshort, y, GLshort, z, GLshort, w);
GLFUNC_4(glVertexPointer, GLint, size, GLenum, type, GLsizei, stride, const void*, pointer);
GLFUNC_4(glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height);
GLFUNC_5(glCopyPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLenum, type);
GLFUNC_5(glDrawPixels, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, const void*, pixels);
GLFUNC_5(glEvalMesh2, GLenum, mode, GLint, i1, GLint, i2, GLint, j1, GLint, j2);
GLFUNC_5(glGetTexImage, GLenum, target, GLint, level, GLenum, format, GLenum, type, void*, pixels);
GLFUNC_6(glCopyTexSubImage1D, GLenum, target, GLint, level, GLint, xoffset, GLint, x, GLint, y, GLsizei, width);
GLFUNC_6(glFrustum, GLdouble, left, GLdouble, right, GLdouble, bottom, GLdouble, top, GLdouble, zNear, GLdouble, zFar);
GLFUNC_6(glMap1d, GLenum, target, GLdouble, u1, GLdouble, u2, GLint, stride, GLint, order, const GLdouble*, points);
GLFUNC_6(glMap1f, GLenum, target, GLfloat, u1, GLfloat, u2, GLint, stride, GLint, order, const GLfloat*, points);
GLFUNC_6(glMapGrid2d, GLint, un, GLdouble, u1, GLdouble, u2, GLint, vn, GLdouble, v1, GLdouble, v2);
GLFUNC_6(glMapGrid2f, GLint, un, GLfloat, u1, GLfloat, u2, GLint, vn, GLfloat, v1, GLfloat, v2);
GLFUNC_6(glOrtho, GLdouble, left, GLdouble, right, GLdouble, bottom, GLdouble, top, GLdouble, zNear, GLdouble, zFar);
GLFUNC_7(glBitmap, GLsizei, width, GLsizei, height, GLfloat, xorig, GLfloat, yorig, GLfloat, xmove, GLfloat, ymove, const GLubyte*, bitmap);
GLFUNC_7(glCopyTexImage1D, GLenum, target, GLint, level, GLenum, internalFormat, GLint, x, GLint, y, GLsizei, width, GLint, border);
GLFUNC_7(glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, void*, pixels);
GLFUNC_7(glTexSubImage1D, GLenum, target, GLint, level, GLint, xoffset, GLsizei, width, GLenum, format, GLenum, type, const void*, pixels);
GLFUNC_8(glCopyTexImage2D, GLenum, target, GLint, level, GLenum, internalFormat, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLint, border);
GLFUNC_8(glCopyTexSubImage2D, GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset, GLint, x, GLint, y, GLsizei, width, GLsizei, height);
GLFUNC_8(glTexImage1D, GLenum, target, GLint, level, GLint, internalformat, GLsizei, width, GLint, border, GLenum, format, GLenum, type, const void*, pixels);

// Intercept all texture allocations so we can save them to file:
// GLFUNC_9(glTexImage2D, GLenum, target, GLint, level, GLint, internalformat, GLsizei, width, GLsizei, height, GLint, border, GLenum, format, GLenum, type, const void*, pixels);
GLPROXY_EXTERN void GLPROXY_DECL glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    static GLProxy::TGLFunc<void, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*> TGLFUNC_DECL(glTexImage2D);

    if (g_RendererIsStarted && War3::DebugUI::dumpTexturesToFile)
    {
        War3::ImageManager::getInstance().texImage2D(target, level, internalformat, width, height, border, format, type, reinterpret_cast<const std::uint8_t*>(pixels));
    }

    TGLFUNC_CALL(glTexImage2D, target, level, internalformat, width, height, border, format, type, pixels);
}

GLFUNC_9(glTexSubImage2D, GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, const void*, pixels);
GLFUNC_10(glMap2d, GLenum, target, GLdouble, u1, GLdouble, u2, GLint, ustride, GLint, uorder, GLdouble, v1, GLdouble, v2, GLint, vstride, GLint, vorder, const GLdouble*, points);
GLFUNC_10(glMap2f, GLenum, target, GLfloat, u1, GLfloat, u2, GLint, ustride, GLint, uorder, GLfloat, v1, GLfloat, v2, GLint, vstride, GLint, vorder, const GLfloat*, points);
