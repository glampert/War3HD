
#pragma once

// ============================================================================
// File:   Window.hpp
// Author: Guilherme R. Lampert
// Brief:  Window hooks / helpers.
// ============================================================================

#include "Common.hpp"

namespace War3
{

// Miscellaneous window hooks and helpers.
class Window final
{
public:
    static void findHandle();
    static void restoreAltTab();
    static void resetDisplayMode();
    static void setWindowed(int w, int h);

    static Size2D getScreenSize();
    static void* getHandle() { return sm_hWnd; } // -> HWND

    static const Size2D kDefaultDebugSize;
    static void installDebugHooks();

    // Input helpers:
    static bool isKeyDown(int vkey);
    static bool isKeyUp(int vkey);

private:
    static void* sm_hWnd;
    static const char* sm_windowName;
};

} // namespace War3
