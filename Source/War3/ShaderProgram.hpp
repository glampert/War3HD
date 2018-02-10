
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

class ShaderProgram final
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
                  const std::string& directives = "");

    // Initialize from vertex and fragment shader source files.
    // Both files must be valid. You can provided additional directives
    // or even code that is added right at the top of each file.
    ShaderProgram(const std::string& vsFile,
                  const std::string& fsFile,
                  const std::string& directives = "");

    // Frees the associated data.
    ~ShaderProgram();

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
    static bool checkShaderInfoLogs(unsigned progHandle, unsigned vsHandle, unsigned fsHandle) noexcept;

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
        delete sm_sharedInstance;
        sm_sharedInstance = nullptr;
    }

    // Shaders available:
    enum ShaderId
    {
        kFramePostProcess,

        kShaderCount // Number of entries in the enum - internal use
    };

    const ShaderProgram& getShader(const ShaderId id) const
    {
        return *m_shaders[id];
    }

private:
    // Singleton instance:
    ShaderProgramManager();
    static ShaderProgramManager* sm_sharedInstance;

    // Shader pool, all entries initialized on startup:
    using ShaderProgPtr = std::unique_ptr<ShaderProgram>;
    std::array<ShaderProgPtr, kShaderCount> m_shaders;
};

} // namespace War3
