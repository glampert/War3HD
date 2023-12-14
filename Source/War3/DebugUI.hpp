
#pragma once

// ============================================================================
// File:   DebugUI.hpp
// Author: Guilherme R. Lampert
// Brief:  ImGui debug overlays.
// ============================================================================

namespace War3
{

struct Size2D;

// [F10] toggles the debug ImGUI menu.
struct DebugUI final
{
    static float scaling;
    static bool isStarted;

    // FXAA post-processing:
    static bool enableFxaa;
    static bool fxaaDebug;

    // Post-processing image filters:
    static bool enableHDR;
    static bool enableBloom;
    static bool enableNoise;
    static bool enablePostProcessing() { return enableFxaa | enableHDR | enableBloom | enableNoise; }

    // Debug shaders:
    static bool debugViewTexCoords;
    static bool debugViewVertNormals;
    static bool debugViewVertColors;
    static bool debugViewVertPositions;
    static bool debugViewPolyOutlines;
    static bool enableDebugShader() { return debugViewTexCoords | debugViewVertNormals | debugViewVertColors | debugViewVertPositions | debugViewPolyOutlines; }

    // Other debug settings:
    static bool dumpTexturesToFile;

    static void start();
    static void stop();
    static void render(const Size2D& screenSize);
    static void logListenerCallback(const char* str);
};

} // namespace War3
