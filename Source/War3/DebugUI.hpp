
#pragma once

// ============================================================================
// File:   DebugUI.hpp
// Author: Guilherme R. Lampert
// Brief:  ImGui debug overlays.
// ============================================================================

namespace War3
{

class DebugUI final
{
public:
    static float sm_scaling;
    static bool  sm_isStarted;

    static void start();
    static void stop();
    static void render();
};

} // namespace War3
