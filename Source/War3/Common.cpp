
// ============================================================================
// File:   Common.cpp
// Author: Guilherme R. Lampert
// Brief:  Miscellaneous shared functions and helpers.
// ============================================================================

#include "Common.hpp"

#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctime>

namespace War3
{

// ========================================================
// LogStream:
// ========================================================

LogStream::LogStream(const char* filename, const bool debugWindow)
    : m_file{ nullptr }
    , m_useDebugWindow{ debugWindow }
{
    fopen_s(&m_file, filename, "wt");
    if (m_file == nullptr)
    {
        MessageBoxA(nullptr, "Unable to open log file!", "Error", MB_OK | MB_ICONWARNING);
    }
}

LogStream::~LogStream()
{
    if (m_file != nullptr)
    {
        std::fclose(m_file);
        m_file = nullptr;
    }
}

void LogStream::write(const char c)
{
    if (m_file != nullptr)
    {
        std::fputc(c, m_file);
    }
    if (m_useDebugWindow)
    {
        const char dbgStr[] = { c, '\0' };
        OutputDebugStringA(dbgStr);
    }
}

void LogStream::write(const char* str)
{
    if (str != nullptr && *str != '\0')
    {
        if (m_file != nullptr)
        {
            std::fputs(str, m_file);
        }
        if (m_useDebugWindow)
        {
            OutputDebugStringA(str);
        }
    }
}

void LogStream::writeV(const char* fmt, va_list vaArgs)
{
    char message[2048] = {'\0'};
    std::vsnprintf(message, sizeof(message), fmt, vaArgs);
    write(message);
}

void LogStream::writeF(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    va_list vaArgs;
    char message[2048] = {'\0'};

    va_start(vaArgs, fmt);
    std::vsnprintf(message, sizeof(message), fmt, vaArgs);
    va_end(vaArgs);

    write(message);
}

void LogStream::flush()
{
    if (m_file != nullptr)
    {
        std::fflush(m_file);
    }
}

// ========================================================
// Debug logging:
// ========================================================

void assertFailure(const char* expr, const char* func, const char* file, const int line)
{
    char message[2048] = {'\0'};
    std::snprintf(message, sizeof(message), "%s in %s, %s(%i)", expr, func, file, line);

    #if WAR3_WITH_LOG
    auto& log = getLogStream();
    log.writeF("ASSERT FAILED: %s\n", message);
    log.flush();
    #endif // WAR3_WITH_LOG

    MessageBoxA(nullptr, message, "Assert Failed", MB_OK | MB_ICONERROR);
    WAR3_DEBUGBREAK();
}

void fatalError(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    va_list vaArgs;
    char message[2048] = {'\0'};

    va_start(vaArgs, fmt);
    std::vsnprintf(message, sizeof(message), fmt, vaArgs);
    va_end(vaArgs);

    #if WAR3_WITH_LOG
    auto& log = getLogStream();
    log.writeF("Terminating due to fatal error: %s\n", message);
    log.flush();
    #endif // WAR3_WITH_LOG

    MessageBoxA(nullptr, message, "War3HD Fatal Error", MB_OK | MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}

#if WAR3_WITH_LOG
LogStream& getLogStream()
{
    static LogStream s_TheLog{ "War3HD.log" };
    return s_TheLog;
}

void error(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    auto& log = getLogStream();
    log.write("ERROR: ");

    va_list vaArgs;
    va_start(vaArgs, fmt);
    log.writeV(fmt, vaArgs);
    va_end(vaArgs);

    log.write('\n');
    log.flush(); // Errors flush the log
}

void warn(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    auto& log = getLogStream();
    log.write("WARN: ");

    va_list vaArgs;
    va_start(vaArgs, fmt);
    log.writeV(fmt, vaArgs);
    va_end(vaArgs);

    log.write('\n');
}

void info(WAR3_PRINTF_LIKE const char* fmt, ...)
{
    auto& log = getLogStream();
    log.write("INFO: ");

    va_list vaArgs;
    va_start(vaArgs, fmt);
    log.writeV(fmt, vaArgs);
    va_end(vaArgs);

    log.write('\n');
}
#endif // WAR3_WITH_LOG

// ========================================================
// Miscellaneous helpers:
// ========================================================

std::string numToString(const std::uint64_t num)
{
    char temp[128];
    std::snprintf(temp, sizeof(temp), "%-10llu", num);
    return temp;
}

std::string ptrToString(const void* ptr)
{
    char temp[128];
    #if defined(_M_IX86)
    std::snprintf(temp, sizeof(temp), "0x%08X", reinterpret_cast<std::uintptr_t>(ptr));
    #elif defined(_M_X64)
    std::snprintf(temp, sizeof(temp), "0x%016llX", reinterpret_cast<std::uintptr_t>(ptr));
    #endif // x86/64
    return temp;
}

std::string getTimeString()
{
    const std::time_t rawTime = std::time(nullptr);

    // Visual Studio dislikes the static buffer returned by regular std::ctime.
    char temp[256] = {'\0'};
    ctime_s(temp, sizeof(temp), &rawTime);

    std::string ts = temp;
    ts.pop_back(); // Remove the default '\n' added by ctime.
    return ts;
}

std::string lastWinErrorAsString()
{
    // Adapted from this SO thread:
    // http://stackoverflow.com/a/17387176/1198654

    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0)
    {
        return "Unknown error";
    }

    LPSTR messageBuffer = nullptr;
    constexpr DWORD fmtFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS;

    const auto size = FormatMessageA(fmtFlags, nullptr, errorMessageID,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                     (LPSTR)&messageBuffer, 0, nullptr);

    const std::string message{ messageBuffer, size };
    LocalFree(messageBuffer);

    return message + "(error " + std::to_string(errorMessageID) + ")";
}

std::string getRealGLLibPath()
{
    char defaultLibDir[1024] = {'\0'};
    GetSystemDirectoryA(defaultLibDir, sizeof(defaultLibDir));

    std::string result;
    if (defaultLibDir[0] != '\0')
    {
        result += defaultLibDir;
        result += "\\opengl32.dll";
    }
    else // Something wrong... Try a hardcoded path for now...
    {
        result = "C:\\windows\\system32\\opengl32.dll";
        warn("GetSystemDirectory returned an empty path, assuming default system32 directory...");
    }
    return result;
}

void* getSelfModuleHandle()
{
    //
    // This is somewhat hackish, but should work.
    // We try to get this module's address from the address
    // of one of its functions, this very function actually.
    // Worst case it fails and we return null.
    //
    // There's also the '__ImageBase' hack, but that seems even more precarious...
    // http://stackoverflow.com/a/6924293/1198654
    //
    HMODULE selfHMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&getSelfModuleHandle,
                       &selfHMod);
    return reinterpret_cast<void*>(selfHMod);
}

// ========================================================

} // namespace War3
