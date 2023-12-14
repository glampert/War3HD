
// ============================================================================
// File:   DebugUI.cpp
// Author: Guilherme R. Lampert
// Brief:  ImGui debug overlays.
// ============================================================================

#include "DebugUI.hpp"
#include "Window.hpp"
#include "ShaderProgram.hpp"
#include "GLProxy/GLDllUtils.hpp"
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

static WNDPROC g_pPrevWndProc{};

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
// War3 ImGui log listener:
// ========================================================

static std::vector<std::string>& getDebugLogStrings()
{
    static std::vector<std::string> s_TheLogStrings;
    return s_TheLogStrings;
}

void DebugUI::logListenerCallback(const char* str)
{
    getDebugLogStrings().push_back(str);
}

// ========================================================
// DebugUI:
// ========================================================

static const char* g_GLVersionStr = "";
static const char* g_GLVendorStr = "";
static const char* g_GLRendererStr = "";
static const char* g_GLSLVersionStr = "";

static bool g_ShowDebugUI = false;
static bool g_ShowGLFunctionStatsWindow = false;
static bool g_ShowWar3LogWindow = false;

float DebugUI::scaling = 1.0f;
bool DebugUI::isStarted = false;

bool DebugUI::enableFxaa = false;
bool DebugUI::fxaaDebug = false;

bool DebugUI::enableHDR = false;
bool DebugUI::enableBloom = false;
bool DebugUI::enableNoise = false;

bool DebugUI::debugViewTexCoords = false;
bool DebugUI::debugViewVertNormals = false;
bool DebugUI::debugViewVertColors = false;
bool DebugUI::debugViewVertPositions = false;
bool DebugUI::debugViewPolyOutlines = false;

bool DebugUI::dumpTexturesToFile = false;

static bool resetDebugViewSettings(const bool savedState)
{
    DebugUI::debugViewTexCoords     = false;
    DebugUI::debugViewVertNormals   = false;
    DebugUI::debugViewVertColors    = false;
    DebugUI::debugViewVertPositions = false;
    DebugUI::debugViewPolyOutlines  = false;

    return savedState;
}

static void renderMainDebugWindow(const Size2D& screenSize)
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("War3HD Debug UI", &g_ShowDebugUI))
    {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Text("Viewport: [W:%d, H:%d]", screenSize.width, screenSize.height);

        ImGui::Separator();

        ImGui::Text("GL_VERSION....: %s", g_GLVersionStr);
        ImGui::Text("GL_VENDOR.....: %s", g_GLVendorStr);
        ImGui::Text("GL_RENDERER...: %s", g_GLRendererStr);
        ImGui::Text("GLSL_VERSION..: %s", g_GLSLVersionStr);

        ImGui::Separator();

        ImGui::Text("Renderer Options:");

        if (ImGui::CollapsingHeader("FXAA", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable FXAA", &DebugUI::enableFxaa);
            ImGui::Checkbox("FXAA Debug", &DebugUI::fxaaDebug);
        }

        if (ImGui::CollapsingHeader("Post-Processing", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable HDR", &DebugUI::enableHDR);
            ImGui::Checkbox("Enable Bloom", &DebugUI::enableBloom);
            ImGui::Checkbox("Enable Noise", &DebugUI::enableNoise);
        }

        if (ImGui::CollapsingHeader("Debug", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Show War3 Log", &g_ShowWar3LogWindow);
            ImGui::Checkbox("Show GL Function Stats", &g_ShowGLFunctionStatsWindow);
            ImGui::Checkbox("Dump Textures To File", &DebugUI::dumpTexturesToFile);

            if (ImGui::Checkbox("View Tex Coords", &DebugUI::debugViewTexCoords))
            {
                DebugUI::debugViewTexCoords = resetDebugViewSettings(DebugUI::debugViewTexCoords);
            }
            if (ImGui::Checkbox("View Vertex Normals", &DebugUI::debugViewVertNormals))
            {
                DebugUI::debugViewVertNormals = resetDebugViewSettings(DebugUI::debugViewVertNormals);
            }
            if (ImGui::Checkbox("View Vertex Colors", &DebugUI::debugViewVertColors))
            {
                DebugUI::debugViewVertColors = resetDebugViewSettings(DebugUI::debugViewVertColors);
            }
            if (ImGui::Checkbox("View Vertex Positions", &DebugUI::debugViewVertPositions))
            {
                DebugUI::debugViewVertPositions = resetDebugViewSettings(DebugUI::debugViewVertPositions);
            }
            if (ImGui::Checkbox("View Polygon Outlines", &DebugUI::debugViewPolyOutlines))
            {
                DebugUI::debugViewPolyOutlines = resetDebugViewSettings(DebugUI::debugViewPolyOutlines);
            }
        }

        saveWindowRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
    }
    ImGui::End();
}

static void renderGLFunctionStatsWindow()
{
    ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 900), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("OpenGL Function Stats", &g_ShowGLFunctionStatsWindow))
    {
        const auto sortedFuncs = GLProxy::getSortedGLFunctions();
        for (const GLProxy::GLFuncBase* func : sortedFuncs)
        {
            ImGui::Text("%s %s", War3::numToString(func->callCount).c_str(), func->name);
        }

        saveWindowRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
    }
    ImGui::End();
}

static void renderWar3LogWindow()
{
    ImGui::SetNextWindowPos(ImVec2(10, 570), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("War3HD Log", &g_ShowWar3LogWindow))
    {
        const ImVec4 kWhite{ 1.0f, 1.0f, 1.0f, 1.0f };
        const ImVec4 kYellow{ 1.0f, 1.0f, 0.0f, 1.0f };
        const ImVec4 kRed{ 1.0f, 0.0f, 0.0f, 1.0f };

        ImVec4 textColor = kWhite;
        std::string textBuffer{};

        auto printCurrentLine = [&]()
        {
            if (!textBuffer.empty())
            {
                ImGui::TextColored(textColor, "%s", textBuffer.c_str());
                textBuffer.clear();
            }
        };

        const auto& logStrings = getDebugLogStrings();
        for (const std::string& str : logStrings)
        {
            if (str.find("INFO:") != std::string::npos)
            {
                printCurrentLine();
                textColor = kWhite;
            }
            else if (str.find("WARN:") != std::string::npos)
            {
                printCurrentLine();
                textColor = kYellow;
            }
            else if (str.find("ERROR:") != std::string::npos)
            {
                printCurrentLine();
                textColor = kRed;
            }
            else
            {
                textBuffer += str;
            }
        }
        printCurrentLine();

        saveWindowRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
    }
    ImGui::End();
}

void DebugUI::start()
{
    if (isStarted)
    {
        return;
    }

    // We won't recreate the ImGui context on every start/stop, only the OpenGL state needs recreating.
    if (ImGui::GetCurrentContext() == nullptr)
    {
        ImGui::CreateContext();

        ImGui::GetStyle().ScaleAllSizes(scaling);
        ImGui::GetIO().FontGlobalScale = scaling;

        hookImGuiWindProc();
    }

    ImGui_ImplWin32_Init(Window::getHandle());
    ImGui_ImplOpenGL3_Init();
    isStarted = true;

    g_GLVersionStr   = (const char*)GLProxy::glGetString(GL_VERSION);
    g_GLVendorStr    = (const char*)GLProxy::glGetString(GL_VENDOR);
    g_GLRendererStr  = (const char*)GLProxy::glGetString(GL_RENDERER);
    g_GLSLVersionStr = ShaderProgram::getGlslVersionDirective().c_str();
}

void DebugUI::stop()
{
    if (!isStarted)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    isStarted = false;

    getDebugLogStrings().clear();
    getDebugLogStrings().shrink_to_fit();
}

void DebugUI::render(const Size2D& screenSize)
{
    if (!isStarted)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(screenSize.width);
    io.DisplaySize.y = static_cast<float>(screenSize.height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    constexpr int kToggleKey = VK_F10;
    static bool s_IsToggleKeyDown = false;

    if (Window::isKeyDown(kToggleKey) && !s_IsToggleKeyDown)
    {
        s_IsToggleKeyDown = true;
    }
    else if (Window::isKeyUp(kToggleKey) && s_IsToggleKeyDown)
    {
        s_IsToggleKeyDown = false;
        g_ShowDebugUI = !g_ShowDebugUI;
    }

    if (g_ShowDebugUI)
    {
        renderMainDebugWindow(screenSize);

        if (g_ShowGLFunctionStatsWindow)
        {
            renderGLFunctionStatsWindow();
        }

        if (g_ShowWar3LogWindow)
        {
            renderWar3LogWindow();
        }
    }

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
    const int displayWidth  = screenSize.width;
    const int displayHeight = screenSize.height;
    GLProxy::glViewport(0, 0, displayWidth, displayHeight);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace War3
