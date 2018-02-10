
// ============================================================================
// File:   Window.cpp
// Author: Guilherme R. Lampert
// Brief:  Window hooks / helpers.
// ============================================================================

#include "Window.hpp"

#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>

#include "detours/detours.h"

extern "C"
{

// Disable cursor clipping to window bounds so we can more easily debug the War3 executable.
BOOL WINAPI War3_ClipCursorNoOp(const RECT* /*pRect*/) { return TRUE; }

// Empty dummy function with no arguments - no registers modified
void __cdecl War3_NoOpFunction0Args(void) { }

} // extern "C"

namespace War3
{

// ========================================================
// Program/DLL address patching helpers:
// ========================================================

static void patchAddressImpl(const std::uintptr_t addr, const std::uintptr_t patch)
{
    volatile std::uintptr_t* patchTarget = (volatile std::uintptr_t*)addr;

    DWORD oldProtect = 0;
    const DWORD newProtect = PAGE_READWRITE;
    if (!VirtualProtect((void*)patchTarget, sizeof(void*), newProtect, &oldProtect))
    {
        error("PatchAddress: VirtualProtect failed for 0x%p!", patchTarget);
        return;
    }

    (*patchTarget) = patch;

    info("Patched address 0x%p with 0x%p, oldProtect was %u",
         (void*)patchTarget, (void*)patch, oldProtect);
}

template<typename Addr, typename Patch>
static void patchAddress(const Addr addr, const Patch patch)
{
    patchAddressImpl((std::uintptr_t)addr, (std::uintptr_t)patch);
}

// ========================================================
// Window:
// ========================================================

void* Window::sm_hWnd = nullptr;
const char* Window::sm_windowName = "Warcraft III";
const Size2D Window::kDefaultDebugSize = { 2160, 1350 };

void Window::findHandle()
{
    EnumWindows(
        [](HWND hWnd, LPARAM /*unused*/) -> BOOL
        {
            DWORD procId = 0;
            GetWindowThreadProcessId(hWnd, &procId);
            if (procId == GetCurrentProcessId())
            {
                char str[1024] = {'\0'};
                GetWindowText(hWnd, str, sizeof(str));
                if (std::strcmp(str, sm_windowName) == 0)
                {
                    Window::sm_hWnd = hWnd;
                    info("Warcraft III window HWND is %s", ptrToString(hWnd).c_str());
                    return false; // Found, stop iterating.
                }
            }
            return true;
        }, (LPARAM)0);

    if (sm_hWnd == nullptr)
    {
        fatalError("Did not find window handle for '%s'!", sm_windowName);
    }
}

void Window::restoreAltTab()
{
    // Clear first 256 hotkey slots to remove the ATL+TAB hotkey set by the game
    for (int i = 0; i < 256; ++i)
    {
        UnregisterHotKey(nullptr, i);
    }
}

void Window::resetDisplayMode()
{
    ChangeDisplaySettings(nullptr, 0);
}

Size2D Window::getScreenSize()
{
    if (sm_hWnd == nullptr)
    {
        fatalError("Call Window::findHandle() first!");
    }

    HMONITOR monitor = MonitorFromWindow((HWND)sm_hWnd, MONITOR_DEFAULTTONEAREST);
    WAR3_ASSERT(monitor != nullptr);

    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);

    const BOOL ok = GetMonitorInfo(monitor, &info);
    WAR3_ASSERT(ok && "GetMonitorInfo failed!");
    (void)ok;

    const long monitorWidth  = info.rcMonitor.right  - info.rcMonitor.left;
    const long monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;

    return { static_cast<int>(monitorWidth),
             static_cast<int>(monitorHeight) };
}

void Window::setWindowed(const int w, const int h)
{
    if (sm_hWnd == nullptr)
    {
        fatalError("Call Window::findHandle() first!");
    }

    RECT newRect;
    newRect.left   = 0;
    newRect.top    = 0;
    newRect.right  = w;
    newRect.bottom = h;

    SetWindowLongPtr((HWND)sm_hWnd, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_BORDER | WS_VISIBLE));
    AdjustWindowRect(&newRect, (WS_OVERLAPPEDWINDOW | WS_BORDER | WS_VISIBLE), FALSE);
    MoveWindow((HWND)sm_hWnd, 0, 0, newRect.right - newRect.left, newRect.bottom - newRect.top, TRUE);

    resetDisplayMode();
}

void Window::installDebugHooks()
{
    Window::findHandle();
    Window::restoreAltTab();
    Window::setWindowed(Window::kDefaultDebugSize.width, Window::kDefaultDebugSize.height);

    // Set up Windows-level system functions detours:
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    auto pfnClipCursor = DetourFindFunction("User32.dll", "ClipCursor");
    info("{Detours} Real ClipCursor addr: 0x%p", reinterpret_cast<void*>(pfnClipCursor));

    long errorCode = DetourAttach(&pfnClipCursor, &War3_ClipCursorNoOp);
    if (errorCode != NO_ERROR)
    {
        error("DetourAttach failed with error %lu", errorCode);
    }

    errorCode = DetourTransactionCommit();
    if (errorCode != NO_ERROR)
    {
        error("DetourTransactionCommit failed with error %lu", errorCode);
    }

    // Patch shutdown crash when attached to the debugger:
    //  - Redirect bad call instruction to a no-op function.
    // FIXME: Very likely dependent on the version of the game EXE!
    patchAddress(0x6F876600, &War3_NoOpFunction0Args);
}

bool Window::isKeyDown(const int vkey)
{
    return (GetAsyncKeyState(vkey) & 0x8000) ? true : false;
}

bool Window::isKeyUp(const int vkey)
{
    return (GetAsyncKeyState(vkey) & 0x8000) ? false : true;
}

} // namespace War3
