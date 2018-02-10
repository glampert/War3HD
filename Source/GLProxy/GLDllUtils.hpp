
#pragma once

// ============================================================================
// File:   GLDllUtils.hpp
// Author: Guilherme R. Lampert
// Brief:  Real OpenGL DLL loader and other DLL interception/stats utils.
// ============================================================================

#include "GLProxy/GLExtensions.hpp"

namespace GLProxy
{

// ========================================================
// class OpenGLDll:
// ========================================================

// Simple helper class to manage loading the real OpenGL
// Dynamic Library and fetching function pointers from it.
class OpenGLDll final
{
public:
    // Not copyable.
    OpenGLDll(const OpenGLDll&) = delete;
    OpenGLDll& operator=(const OpenGLDll&) = delete;

    OpenGLDll();
    ~OpenGLDll();

    void load();
    void unload();
    bool isLoaded() const noexcept;
    void* getFuncPtr(const char* funcName) const;

    static OpenGLDll& getInstance();
    static void* getRealGLFunc(const char* funcName);

private:
    HMODULE m_dllHandle;
    std::string m_dllFilePath;
};

// ========================================================
// GL function pointers database:
// ========================================================

// Registry for the real functions from the GL DLL.
struct GLFuncBase
{
    std::uint64_t     callCount; // Times called during program lifetime.
    const char*       name;      // Pointer to static string. OGL function name, like "glEnable".
    const GLFuncBase* next;      // Pointer to next static instance in the global list.
};

// Linked list of TGLFuncs, pointing to the actual OpenGL DLL methods.
extern GLFuncBase* g_RealGLFunctions;

// Each function requires a different signature.
// These are always declared as static instances.
template<typename RetType, typename... ParamTypes>
struct TGLFunc final : GLFuncBase
{
    using PtrType = RetType (GLPROXY_DECL*)(ParamTypes...);
    PtrType funcPtr; // Pointer to a GL function inside the actual OpenGL DLL.

    explicit TGLFunc(const char* funcName)
    {
        callCount = 0;
        name      = funcName;
        funcPtr   = static_cast<PtrType>(OpenGLDll::getRealGLFunc(funcName));

        // Link to the global list:
        this->next = g_RealGLFunctions;
        g_RealGLFunctions = this;
    }
};

// Shorthand macros to simplify the TGLFunc<> declarations:
#define TGLFUNC_NAME(funcName) WAR3_STRING_JOIN2(s_real_, funcName)
#define TGLFUNC_DECL(funcName) TGLFUNC_NAME(funcName){ WAR3_STRINGIZE(funcName) }
#define TGLFUNC_CALL(funcName, ...) (TGLFUNC_NAME(funcName).callCount++, TGLFUNC_NAME(funcName).funcPtr(__VA_ARGS__))

// ========================================================

// Writes a report with the OpenGL function call
// counts in the destructor (or via direct call to writeReport).
#if GLPROXY_WITH_LOG
struct AutoReport final
{
    AutoReport();
    ~AutoReport();
    static void writeReport();
};
#endif // GLPROXY_WITH_LOG

// ========================================================

} // namespace GLProxy
