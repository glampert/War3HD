
#pragma once

// ============================================================================
// File:   Common.hpp
// Author: Guilherme R. Lampert
// Brief:  Miscellaneous shared definitions and functions.
// ============================================================================

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <intrin.h>
#include <memory>
#include <string>
#include <utility>
#include <functional>

// Microsoft's own flavor of format string checking.
// This sadly won't even be used by the compiler, only the static analyzer cares about it,
// so you only get the warnings when building with '/analyze'. Still better than nothing.
#include <sal.h>

#define WAR3_PRINTF_LIKE     _Printf_format_string_
#define WAR3_ATTRIB_NORETURN __declspec(noreturn)
#define WAR3_ATTRIB_NOINLINE __declspec(noinline)

// Appends a pair tokens into a single name/identifier.
// Normally used to declared internal/built-in functions and variables.
#define WAR3_STRING_JOIN2_HELPER(a, b) a ## b
#define WAR3_STRING_JOIN2(a, b) WAR3_STRING_JOIN2_HELPER(a, b)

// Code/text to string
#define WAR3_STRINGIZE_HELPER(str) #str
#define WAR3_STRINGIZE(str) WAR3_STRINGIZE_HELPER(str)

#ifdef NDEBUG
    #define WAR3_ASSERT(expr) /*nothing*/
    #define WAR3_DEBUGBREAK() std::abort()
#else // NDEBUG
    #define WAR3_ASSERT(expr) ((void)((expr) || (War3::assertFailure(#expr, __FUNCTION__, __FILE__, __LINE__), 0)))
    #define WAR3_DEBUGBREAK() __debugbreak()
#endif // NDEBUG

namespace War3
{

// ========================================================

template<typename F>
struct ScopeExitType final
{
    F m_fn;
    ScopeExitType(F f) : m_fn(std::move(f)) {}
    ~ScopeExitType() { m_fn(); }
};

template<typename F>
inline ScopeExitType<F> makeScopeExit(F f) { return ScopeExitType<F>(std::move(f)); }

#define WAR3_SCOPE_EXIT(codeBlock) \
    auto WAR3_STRING_JOIN2(my_scope_exit_, __LINE__) = makeScopeExit([=]() mutable { codeBlock })

// ========================================================

struct Size2D final
{
    int width;
    int height;
};

struct Size3D final
{
    int width;
    int height;
    int depth;
};

// Length in elements of type 'T' of statically allocated C++ arrays.
template<typename T, int N>
constexpr int arrayLength(const T (&)[N]) noexcept
{
    return N;
}

// Clamp any value within minimum/maximum range, inclusive.
template<typename T>
constexpr T clamp(const T& x, const T& minimum, const T& maximum) noexcept
{
    return (x < minimum) ? minimum : (x > maximum) ? maximum : x;
}

// ========================================================

class LogStream final
{
public:
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    explicit LogStream(const char* filename, bool debugWindow = true, const std::function<void(const char*)>& logListener = {});
    ~LogStream();

    void write(char c);
    void write(const char* str);
    void writeV(const char* fmt, va_list vaArgs);
    void writeF(WAR3_PRINTF_LIKE const char* fmt, ...);
    void flush();

private:
    FILE* m_file;
    const bool m_useDebugWindow;
    const std::function<void(const char*)> m_logListener;
};

// ========================================================

// Strips out all logging when zero.
#define WAR3_WITH_LOG 1

// Report assert failure with log+popup and aborts.
WAR3_ATTRIB_NORETURN
WAR3_ATTRIB_NOINLINE
void assertFailure(const char* expr, const char* func, const char* file, int line);

// Log fatal error, flush log and exit with failure code.
WAR3_ATTRIB_NORETURN
WAR3_ATTRIB_NOINLINE
void fatalError(WAR3_PRINTF_LIKE const char* fmt, ...);

// Log printing helpers:
#if WAR3_WITH_LOG
    LogStream& getLogStream();
    void error(WAR3_PRINTF_LIKE const char* fmt, ...);
    void warn(WAR3_PRINTF_LIKE const char* fmt, ...);
    void info(WAR3_PRINTF_LIKE const char* fmt, ...);
#else // WAR3_WITH_LOG
    #pragma warning(disable: 4714) // "function marked as __forceinline not inlined"
    __forceinline void error(const char*, ...) { /* nothing */ }
    __forceinline void warn(const char*,  ...) { /* nothing */ }
    __forceinline void info(const char*,  ...) { /* nothing */ }
#endif // WAR3_WITH_LOG

// ========================================================

// Miscellaneous utility functions:
std::string numToString(const std::uint64_t num);
std::string ptrToString(const void* ptr);
std::string getTimeString();
std::string lastWinErrorAsString();
std::string getRealGLLibPath();
void* getSelfModuleHandle(); // -> HMODULE
void createDirectories(const std::string& path);

// ========================================================

} // namespace War3
