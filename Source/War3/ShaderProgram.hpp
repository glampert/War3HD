
// ================================================================================================
// -*- C++ -*-
// File: ShaderProgram.hpp
// Author: Guilherme R. Lampert
// Created on: 26/11/15
// Brief: Shader Program management helpers.
// ================================================================================================

#ifndef WAR3_SHADER_PROGRAM_HPP
#define WAR3_SHADER_PROGRAM_HPP

#include "War3/Common.hpp"
#include <vector>
#include <array>

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
    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram & operator = (const ShaderProgram &) = delete;

    // Movable.
    ShaderProgram(ShaderProgram && other) noexcept;
    ShaderProgram & operator = (ShaderProgram && other) noexcept;

    // Initialize from the text contents of a shader source file.
    ShaderProgram(TextBuffer && vsSrcText,
                  TextBuffer && fsSrcText,
                  const std::string & directives = "");

    // Initialize from vertex and fragment shader source files.
    // Both files must be valid. You can provided additional directives
    // or even code that is added right at the top of each file.
    ShaderProgram(const std::string & vsFile,
                  const std::string & fsFile,
                  const std::string & directives = "");

    // Frees the associated data.
    ~ShaderProgram();

    // Binds the shader program object.
    void bind() const noexcept;

    // Binds the null/default program (0).
    static void bindNull() noexcept;

    // Handle to the currently bound GL render program.
    static UInt getCurrentGLProgram() noexcept { return currentProg; }

    // Returns the best GLSL #version supported by the platform.
    static const std::string & getGlslVersionDirective();

    // Uniform var handle. Return a negative number if not found.
    int getUniformLocation(const std::string & uniformName) const noexcept;

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
    void setUniformMat3(int loc, const float * m) const noexcept;
    void setUniformMat4(int loc, const float * m) const noexcept;

    // Miscellaneous:
    bool isLinked() const noexcept { return linkedOk; }
    bool isValid()  const noexcept { return handle != 0 && linkedOk; }
    bool isBound()  const noexcept { return handle == currentProg;   }

private:

    void releaseGLHandle() noexcept;
    void invalidate() noexcept;

    static TextBuffer loadShaderFile(const std::string & filename);
    static const char * findShaderIncludes(const char * srcText, std::vector<std::string> & includeFiles);
    static bool checkShaderInfoLogs(UInt progHandle, UInt vsHandle, UInt fsHandle) noexcept;

    // OpenGL program handle.
    UInt handle;

    // Set if GL_LINK_STATUS returned GL_TRUE.
    bool linkedOk;

    // Current OpenGL program enabled or 0 for none.
    static UInt currentProg;

    // Shared by all programs. Set when the first program is created.
    static std::string glslVersionDirective;
};

// ========================================================
// class ShaderProgramManager:
// ========================================================

class ShaderProgramManager final
{
public:

    // Not copyable.
    ShaderProgramManager(const ShaderProgramManager &) = delete;
    ShaderProgramManager & operator = (const ShaderProgramManager &) = delete;

    // Singleton lifetime and access:
    static WAR3_INLINE ShaderProgramManager & getInstance()
    {
        if (sharedInstance == nullptr)
        {
            sharedInstance = new ShaderProgramManager{};
        }
        return *sharedInstance;
    }

    static WAR3_INLINE void deleteInstance()
    {
        delete sharedInstance;
        sharedInstance = nullptr;
    }

    // Shaders available:
    enum ShaderId
    {
        FramePostProcess,
        Count // Internal use
    };

    WAR3_INLINE const ShaderProgram & getShader(const ShaderId id) const
    {
        return *shaders[static_cast<int>(id)];
    }

private:

    // Singleton instance:
    ShaderProgramManager();
    static ShaderProgramManager * sharedInstance;

    // Shader pool, all entries initialized on startup:
    using ShaderProgPtr = std::unique_ptr<ShaderProgram>;
    static constexpr int SPCount = static_cast<int>(ShaderId::Count);
    std::array<ShaderProgPtr, SPCount> shaders;
};

} // namespace War3 {}

#endif // WAR3_SHADER_PROGRAM_HPP
