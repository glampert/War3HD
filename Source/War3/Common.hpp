
// ================================================================================================
// -*- C++ -*-
// File: Common.hpp
// Author: Guilherme R. Lampert
// Created on: 25/11/15
// Brief: Miscellaneous shared definitions and functions.
// ================================================================================================

#ifndef WAR3_COMMON_HPP
#define WAR3_COMMON_HPP

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <memory>
#include <string>

#define NOGDI
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//TEMP used by the log only
#include <iostream>
#include <fstream>

#define WAR3_INLINE __forceinline

#define WAR3_HELPER_TOKEN_APPEND2(x, y) x ## y
#define WAR3_TOKEN_APPEND2(x, y) WAR3_HELPER_TOKEN_APPEND2(x, y)

namespace War3
{

//---------------------------

template<class F>
struct ScopeExitType
{
    F fn;
    ScopeExitType(F f) : fn(f) { }
    ~ScopeExitType() { fn(); }
};

template<class F>
inline ScopeExitType<F> makeScopeExit(F f) noexcept { return ScopeExitType<F>(f); }

#define WAR3_SCOPE_EXIT(codeBlock) \
    auto WAR3_TOKEN_APPEND2(my_scope_exit_, __LINE__) = makeScopeExit([=]() mutable { codeBlock })

//---------------------------

using UByte = std::uint8_t;
using UInt  = unsigned int;

// Length in elements of type 'T' of statically allocated C++ arrays.
template<class T, int N>
constexpr int arrayLength(const T (&)[N]) noexcept
{
	return N;
}

// Clamp any value within minimum/maximum range, inclusive.
template<class T>
constexpr T clamp(const T & x, const T & minimum, const T & maximum) noexcept
{
	return (x < minimum) ? minimum : (x > maximum) ? maximum : x;
}

//TEMPORARY
DECLSPEC_NORETURN inline void fatalError(const std::string & message) noexcept
{
    MessageBoxA(nullptr, message.c_str(), "War3HD Fatal Error", MB_OK | MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}

//FIXME temp (remember to add a timestamp to the log !)
inline static std::ofstream & getLogStream()
{
    static std::ofstream theLog{ "War3HD.log" };
    return theLog;
}

inline void error(const std::string & message) noexcept
{
    getLogStream() << "ERROR: " << message << std::endl;
}

inline void warning(const std::string & message) noexcept
{
    getLogStream() << "WARN: " << message << std::endl;
}

inline void info(const std::string & message) noexcept
{
    getLogStream() << "INFO: " << message << "\n";
}

} // namespace War3 {}

#endif // WAR3_COMMON_HPP
