
// ============================================================================
// File:   DebugUI.cpp
// Author: Guilherme R. Lampert
// Brief:  ImGui debug overlays.
// ============================================================================

#include "DebugUI.hpp"
#include "Window.hpp"
#include "GLProxy/GLExtensions.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_opengl3.h"

namespace War3
{

// ========================================================
// Saved ImGui window rects:
// ========================================================

struct WindowRect
{
    ImVec2 mins;
    ImVec2 maxs;
};

static int g_NumSavedImGuiWindowRects = 0;
static WindowRect g_SavedImGuiWindowRects[16] = {};

static void saveWindowRect(ImVec2 pos, ImVec2 size)
{
    WAR3_ASSERT(g_NumSavedImGuiWindowRects < arrayLength(g_SavedImGuiWindowRects));
    auto& r = g_SavedImGuiWindowRects[g_NumSavedImGuiWindowRects++];

    // add a couple pixels to each side of the box for better mouse hover checks
    r.mins = ImVec2(pos.x - 2, pos.y - 2);
    r.maxs = ImVec2(pos.x + size.x + 2, pos.y + size.y + 2);
}

// ========================================================
// WindProc ImGui detour:
// ========================================================

static WNDPROC g_pPrevWndProc;

static LRESULT CALLBACK wndProcImGuiDetour(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

    if (g_pPrevWndProc)
    {
        return CallWindowProc(g_pPrevWndProc, hWnd, msg, wParam, lParam);
    }
    else
    {
        return 0;
    }
}

static void hookImGuiWindProc()
{
    HWND hWnd = (HWND)Window::getHandle();

    g_pPrevWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    if (!g_pPrevWndProc)
    {
        warn("GetWindowLongPtr(WNDPROC) failed: %s", lastWinErrorAsString().c_str());
    }

    auto result = SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&wndProcImGuiDetour);
    if (!result)
    {
        warn("SetWindowLongPtr(WNDPROC) failed: %s", lastWinErrorAsString().c_str());
    }
}

// ========================================================
// DebugUI:
// ========================================================

float DebugUI::sm_scaling   = 2.0f;
bool  DebugUI::sm_isStarted = false;

void DebugUI::start()
{
    if (sm_isStarted)
    {
        return;
    }

    // We won't recreate the ImGui context on every start/stop, only the OpenGL state needs recreating.
    if (ImGui::GetCurrentContext() == nullptr)
    {
        ImGui::CreateContext();

        ImGui::GetStyle().ScaleAllSizes(sm_scaling);
        ImGui::GetIO().FontGlobalScale = sm_scaling;

        hookImGuiWindProc();
    }

    ImGui_ImplWin32_Init(Window::getHandle());
    ImGui_ImplOpenGL3_Init();
    sm_isStarted = true;
}

void DebugUI::stop()
{
    if (!sm_isStarted)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    sm_isStarted = false;
}

void DebugUI::render()
{
    if (!sm_isStarted)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(Window::getScreenSize().width);
    io.DisplaySize.y = static_cast<float>(Window::getScreenSize().height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ---- TEST BEGIN ----
    {
        // Copied from the ImGui samples:
        static bool show_another_window = true;
        static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        {
            ImGui::SetNextWindowPos(ImVec2(50, 200), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(800, 900), ImGuiCond_FirstUseEver);

            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            saveWindowRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
            ImGui::End();
        }

        if (show_another_window)
        {
            ImGui::SetNextWindowPos(ImVec2(250, 500), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);

            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;

            saveWindowRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
            ImGui::End();
        }
    }
    // ---- TEST END ----

    // Draw mouse cursor if hovering an overlay window
    io.MouseDrawCursor = false;
    for (int i = 0; i < g_NumSavedImGuiWindowRects; ++i)
    {
        const auto& r = g_SavedImGuiWindowRects[i];
        if (ImGui::IsMouseHoveringRect(r.mins, r.maxs, false))
        {
            io.MouseDrawCursor = true;
            break;
        }
    }
    g_NumSavedImGuiWindowRects = 0;

    // Rendering
    ImGui::Render();
    const int displayWidth  = Window::getScreenSize().width;
    const int displayHeight = Window::getScreenSize().height;
    GLProxy::glViewport(0, 0, displayWidth, displayHeight);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace War3
