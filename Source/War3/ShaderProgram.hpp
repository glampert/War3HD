
#pragma once

// ============================================================================
// File:   ShaderProgram.hpp
// Author: Guilherme R. Lampert
// Brief:  Shader Program management helpers.
// ============================================================================

#include "War3/Common.hpp"
#include <array>
#include <vector>

namespace War3
{

// ========================================================
// class ShaderProgram:
// ========================================================

class ShaderProgram
{
public:
    using TextBuffer = std::unique_ptr<char[]>;

    // Not copyable.
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Movable.
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    // Initialize from the text contents of a shader source file.
    ShaderProgram(TextBuffer&& vsSrcText,
                  TextBuffer&& fsSrcText,
                  TextBuffer&& gsSrcText,
                  const std::string& directives,
                  const std::initializer_list<std::string>& optDebugFileNames = {});

    // Initialize from Vertex, Fragment and optional Geometry shader source files.
    // Both VS and FS files must be valid. You can provided additional directives
    // or even code that is added right at the top of each file.
    ShaderProgram(const std::string& vsFile,
                  const std::string& fsFile,
                  const std::string& gsFile,
                  const std::string& directives);

    // Frees the associated data.
    virtual ~ShaderProgram();

    // Binds the shader program object.
    void bind() const noexcept;

    // Binds the null/default program (0).
    static void bindNull() noexcept;

    // Handle to the currently bound GL render program.
    static unsigned getCurrentGLProgram() noexcept { return sm_currentProg; }

    // Returns the best GLSL #version supported by the platform.
    static const std::string& getGlslVersionDirective();

    // Uniform var handle. Return a negative number if not found.
    int getUniformLocation(const std::string& uniformName) const noexcept;

    // uniform int/vecX
    void setUniform1i(int loc, int x) const noexcept;
    void setUniform2i(int loc, int x, int y) const noexcept;
    void setUniform3i(int loc, int x, int y, int z) const noexcept;
    void setUniform4i(int loc, int x, int y, int z, int w) const noexcept;

    // uniform float/vecX
    void setUniform1f(int loc, float x) const noexcept;
    void setUniform2f(int loc, float x, float y) const noexcept;
    void setUniform3f(int loc, float x, float y, float z) const noexcept;
    void setUniform4f(int loc, float x, float y, float z, float w) const noexcept;

    // uniform matNxM
    void setUniformMat3(int loc, const float* m) const noexcept;
    void setUniformMat4(int loc, const float* m) const noexcept;

    // Program parameters / Geometry Shader
    void setProgramParameter(unsigned paramId, int value) const noexcept;
    void setGeometryInputType(int type) const noexcept;
    void setGeometryOutputType(int type) const noexcept;
    void setGeometryOutputVertexCount(unsigned count) const noexcept;

    // Miscellaneous:
    bool isLinked() const noexcept { return m_linkedOk; }
    bool isValid()  const noexcept { return m_handle != 0 && m_linkedOk; }
    bool isBound()  const noexcept { return m_handle == sm_currentProg; }

    // Built-in shaders file search path
    static const char* getShaderPath() noexcept;

private:
    void releaseGLHandle() noexcept;
    void invalidate() noexcept;

    static TextBuffer loadShaderFile(const std::string& filename);
    static const char* findShaderIncludes(const char* srcText, std::vector<std::string>& includeFiles);
    static bool checkShaderInfoLogs(unsigned progHandle, unsigned vsHandle, unsigned fsHandle, unsigned gsHandle,
                                    const std::initializer_list<std::string>& optDebugFileNames) noexcept;

private:
    // OpenGL program handle.
    unsigned m_handle;

    // Set if GL_LINK_STATUS returned GL_TRUE.
    bool m_linkedOk;

    // Current OpenGL program enabled or 0 for none.
    static unsigned sm_currentProg;

    // Shared by all programs. Set when the first program is created.
    static std::string sm_glslVersionDirective;
};

// ========================================================
// class PostProcessShaderProgram:
// ========================================================

class PostProcessShaderProgram : public ShaderProgram
{
public:
    static constexpr const char* kFXAASettings = 
        "#define FXAA_GLSL_130 1\n"
        "#define FXAA_PRESET 5\n"; // Preset 3 is the default

    static constexpr const char* kPostProcessVertexShader = "FullScreenQuad.vert";

    explicit PostProcessShaderProgram(const std::string& fsFile, const std::string& directives = "")
        : ShaderProgram{ /*vsFile=*/kPostProcessVertexShader, fsFile, /*gsFile=*/"", /*directives=*/std::string(kFXAASettings) + directives }
    {
        cacheUniformLocations();
    }

    // Must match the same constants in FramePostProcess.frag
    enum PostProcessFlags : int
    {
        kPostProcessNone  = 0,
        kPostProcessFXAA  = 1 << 1,
        kPostProcessHDR   = 1 << 2,
        kPostProcessBloom = 1 << 3,
        kPostProcessNoise = 1 << 4
    };

    void setPostProcessFlags(const PostProcessFlags flags) const noexcept
    {
        if (m_postProcessFlagsLocation >= 0) // optional
        {
            setUniform1i(m_postProcessFlagsLocation, flags);
        }
    }

    void setScreenSize(const Size2D& screenSize) const noexcept
    {
        if (m_rcpScreenSizeLocation >= 0) // optional
        {
            setUniform2f(m_rcpScreenSizeLocation,
                1.0f / static_cast<float>(screenSize.width),
                1.0f / static_cast<float>(screenSize.height));
        }
    }

    void setColorRenderTargetSlot(const int slot) const noexcept
    {
        WAR3_ASSERT(m_colorRenderTargetLocation >= 0); // required
        setUniform1i(m_colorRenderTargetLocation, slot);
    }

private:
    void cacheUniformLocations()
    {
        m_postProcessFlagsLocation = getUniformLocation("u_PostProcessFlags");
        if (m_postProcessFlagsLocation < 0) // optional
        {
            warn("Cannot find shader variable 'u_PostProcessFlags'");
        }

        m_rcpScreenSizeLocation = getUniformLocation("u_RcpScreenSize");
        if (m_rcpScreenSizeLocation < 0) // optional
        {
            warn("Cannot find shader variable 'u_RcpScreenSize'");
        }

        m_colorRenderTargetLocation = getUniformLocation("u_ColorRenderTarget");
        if (m_colorRenderTargetLocation < 0) // required
        {
            fatalError("Cannot find shader variable 'u_ColorRenderTarget'");
        }
    }

    // Cached uniform locations.
    int m_postProcessFlagsLocation{ -1 };
    int m_rcpScreenSizeLocation{ -1 };
    int m_colorRenderTargetLocation{ -1 };
};

// ========================================================
// class FXAADebugShaderProgram:
// ========================================================

class FXAADebugShaderProgram final : public PostProcessShaderProgram
{
public:
    static constexpr const char* kFXAADebugSettings = "#define FXAA_DEBUG_HORZVERT 1\n";

    explicit FXAADebugShaderProgram(const std::string& fsFile)
        : PostProcessShaderProgram{ fsFile, /*directives=*/kFXAADebugSettings }
    {}
};

// ========================================================
// class DebugShaderProgram:
// ========================================================

class DebugShaderProgram final : public ShaderProgram
{
public:
    DebugShaderProgram(const std::string& vsFile, const std::string& fsFile, const std::string& gsFile)
        : ShaderProgram{ vsFile, fsFile, gsFile, /*directives=*/"" }
    {
        cacheUniformLocations();
    }

    // Must match the same constants in Debug.frag
    enum DebugViewId : int
    {
        kDebugViewNone          = 0,
        kDebugViewTexCoords     = 1,
        kDebugViewVertNormals   = 2,
        kDebugViewVertColors    = 3,
        kDebugViewVertPositions = 4,
        kDebugViewPolyOutlines  = 5
    };

    void setDebugView(const DebugViewId view) const noexcept
    {
        WAR3_ASSERT(m_debugViewLocation >= 0); // required
        setUniform1i(m_debugViewLocation, view);
    }

    void setScreenSize(const Size2D& screenSize) const noexcept
    {
        WAR3_ASSERT(m_screenSizeLocation >= 0); // required
        setUniform2f(m_screenSizeLocation,
            static_cast<float>(screenSize.width),
            static_cast<float>(screenSize.height));
    }

private:
    void cacheUniformLocations()
    {
        m_debugViewLocation = getUniformLocation("u_DebugView");
        if (m_debugViewLocation < 0) // required
        {
            fatalError("Cannot find shader variable 'u_DebugView'");
        }

        m_screenSizeLocation = getUniformLocation("u_ScreenSize");
        if (m_screenSizeLocation < 0) // required
        {
            fatalError("Cannot find shader variable 'u_ScreenSize'");
        }
    }

    // Cached uniform locations.
    int m_debugViewLocation{ -1 };
    int m_screenSizeLocation{ -1 };
};

// ========================================================
// class ShaderProgramManager:
// ========================================================

class ShaderProgramManager final
{
public:
    ShaderProgramManager(const ShaderProgramManager&) = delete;
    ShaderProgramManager& operator=(const ShaderProgramManager&) = delete;

    static ShaderProgramManager& getInstance()
    {
        if (sm_sharedInstance == nullptr)
        {
            sm_sharedInstance = new ShaderProgramManager{};
        }
        return *sm_sharedInstance;
    }

    static void deleteInstance()
    {
        ShaderProgram::bindNull();

        delete sm_sharedInstance;
        sm_sharedInstance = nullptr;
    }

    // Shaders available:
    enum ShaderId : int
    {
        kPresentFramebuffer,
        kFramePostProcess,
        kFXAADebug,
        kDebug,

        kShaderCount // Number of entries in the enum - internal use
    };

    template<typename T = ShaderProgram>
    const T& getShader(const ShaderId id) const
    {
        auto* sp = m_shaders[id].get();
        WAR3_ASSERT(dynamic_cast<const T*>(sp) != nullptr); // ensure casting to correct type.
        return *static_cast<const T*>(sp);
    }

private:
    struct ShaderProgramFactory;

    // Singleton instance:
    ShaderProgramManager();
    static ShaderProgramManager* sm_sharedInstance;

    // Shader pool, all entries initialized on startup:
    using ShaderProgPtr = std::unique_ptr<ShaderProgram>;
    std::array<ShaderProgPtr, kShaderCount> m_shaders;
};

} // namespace War3
