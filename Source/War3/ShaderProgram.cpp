
// ================================================================================================
// -*- C++ -*-
// File: ShaderProgram.cpp
// Author: Guilherme R. Lampert
// Created on: 26/11/15
// Brief: Shader Program management helpers.
// ================================================================================================

#include "War3/ShaderProgram.hpp"
#include "GLProxy/GLExtensions.hpp"
#include <cstdio>  // For std::fopen/sscanf
#include <cstring> // For std::strstr

namespace War3
{

// ========================================================
// WAR3_SHADER_PATH:
// ========================================================

// Built-in shaders file search path:
#ifndef WAR3_SHADER_PATH
    #define WAR3_SHADER_PATH "NewShaders\\"
#endif // WAR3_SHADER_PATH

// ========================================================
// class ShaderProgram:
// ========================================================

UInt ShaderProgram::currentProg{};
std::string ShaderProgram::glslVersionDirective{};

ShaderProgram::ShaderProgram(ShaderProgram && other) noexcept
    : handle   { other.handle   }
    , linkedOk { other.linkedOk }
{
    other.invalidate();
}

ShaderProgram & ShaderProgram::operator = (ShaderProgram && other) noexcept
{
    releaseGLHandle();

    handle   = other.handle;
    linkedOk = other.linkedOk;

    other.invalidate();
    return *this;
}

ShaderProgram::ShaderProgram(TextBuffer && vsSrcText, TextBuffer && fsSrcText, const std::string & directives)
    : handle   { 0 }
    , linkedOk { false }
{
    GLProxy::initializeExtensions();

    if (vsSrcText == nullptr)
    {
        warning("Null Vertex Shader source!");
        return;
    }
    if (fsSrcText == nullptr)
    {
        warning("Null Fragment Shader source!");
        return;
    }

    //
    // Shader #include resolution:
    //

    std::string vsIncludedText;
    std::string fsIncludedText;
    std::vector<std::string> vsIncludes;
    std::vector<std::string> fsIncludes;

    const char * vsSrcTextPtr = findShaderIncludes(vsSrcText.get(), vsIncludes);
    const char * fsSrcTextPtr = findShaderIncludes(fsSrcText.get(), fsIncludes);

    for (const auto & incFile : vsIncludes)
    {
        info("Loading Vertex Shader include \"" + incFile + "\"...");
        auto fileContents = loadShaderFile(incFile);
        vsIncludedText += (fileContents != nullptr ? fileContents.get() : "");
    }

    for (const auto & incFile : fsIncludes)
    {
        info("Loading Fragment Shader include \"" + incFile + "\"...");
        auto fileContents = loadShaderFile(incFile);
        fsIncludedText += (fileContents != nullptr ? fileContents.get() : "");
    }

    //
    // GL handle allocation:
    //

    const auto glProgHandle = GLProxy::glCreateProgram();
    if (glProgHandle == 0)
    {
        warning("Failed to allocate a new GL Program handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
        return;
    }

    const auto glVsHandle = GLProxy::glCreateShader(GL_VERTEX_SHADER);
    const auto glFsHandle = GLProxy::glCreateShader(GL_FRAGMENT_SHADER);
    if (glVsHandle == 0 || glFsHandle == 0)
    {
        warning("Failed to allocate a new GL Shader handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
        return;
    }

    // Vertex shader:
    const char * vsSrcStrings[]{ getGlslVersionDirective().c_str(), directives.c_str(), vsIncludedText.c_str(), vsSrcTextPtr };
    GLProxy::glShaderSource(glVsHandle, arrayLength(vsSrcStrings), vsSrcStrings, nullptr);
    GLProxy::glCompileShader(glVsHandle);
    GLProxy::glAttachShader(glProgHandle, glVsHandle);

    // Fragment shader:
    const char * fsSrcStrings[]{ getGlslVersionDirective().c_str(), directives.c_str(), fsIncludedText.c_str(), fsSrcTextPtr };
    GLProxy::glShaderSource(glFsHandle, arrayLength(fsSrcStrings), fsSrcStrings, nullptr);
    GLProxy::glCompileShader(glFsHandle);
    GLProxy::glAttachShader(glProgHandle, glFsHandle);

    // Link the Shader Program then check and print the info logs, if any.
    GLProxy::glLinkProgram(glProgHandle);
    linkedOk = checkShaderInfoLogs(glProgHandle, glVsHandle, glFsHandle);

    // After a program is linked the shader objects can be safely detached and deleted.
    // Also recommended to save on the memory that would be wasted by keeping the shaders alive.
    GLProxy::glDetachShader(glProgHandle, glVsHandle);
    GLProxy::glDetachShader(glProgHandle, glFsHandle);
    GLProxy::glDeleteShader(glVsHandle);
    GLProxy::glDeleteShader(glFsHandle);

    // OpenGL likes to defer GPU resource allocation to the first time
    // an object is bound to the current state. Binding it now should
    // "warm up" the resource and avoid lag on the first frame rendered with it.
    currentProg = glProgHandle;
    GLProxy::glUseProgram(glProgHandle);

    // Done, log errors and store the handle one way or the other.
    GLPROXY_CHECK_GL_ERRORS();
    handle = glProgHandle;
}

ShaderProgram::ShaderProgram(const std::string & vsFile, const std::string & fsFile, const std::string & directives)
    : ShaderProgram { loadShaderFile(vsFile), loadShaderFile(fsFile), directives }
{
    // Just forwards to the other constructor...
    info("New ShaderProgram created from \"" + vsFile + "\" and \"" + fsFile + "\".");
}

ShaderProgram::~ShaderProgram()
{
    releaseGLHandle();
}

void ShaderProgram::releaseGLHandle() noexcept
{
    if (handle != 0)
    {
        if (handle == currentProg)
        {
            bindNull();
        }

        GLProxy::glDeleteProgram(handle);
        linkedOk = false;
        handle   = 0;
    }
}

void ShaderProgram::invalidate() noexcept
{
    linkedOk = false;
    handle   = 0;
}

ShaderProgram::TextBuffer ShaderProgram::loadShaderFile(const std::string & filename)
{
    if (filename.empty())
    {
        warning("Empty filename in ShaderProgram::loadShaderFile()!");
        return nullptr;
    }

    FILE * fileIn = nullptr;
    const auto fillPath = WAR3_SHADER_PATH + filename;

    WAR3_SCOPE_EXIT(
        if (fileIn != nullptr)
        {
            std::fclose(fileIn);
        }
    );

    #ifdef _MSC_VER
    fopen_s(&fileIn, fillPath.c_str(), "rb");
    #else // !_MSC_VER
    fileIn = std::fopen(fillPath.c_str(), "rb");
    #endif // _MSC_VER

    if (fileIn == nullptr)
    {
        warning("Can't open shader file \"" + fillPath + "\"!");
        return nullptr;
    }

    std::fseek(fileIn, 0, SEEK_END);
    const auto fileLength = std::ftell(fileIn);
    std::fseek(fileIn, 0, SEEK_SET);

    if (fileLength <= 0 || std::ferror(fileIn))
    {
        warning("Error getting length or empty shader file! \"" + fillPath + "\".");
        return nullptr;
    }

    TextBuffer fileContents{ new char[fileLength + 1] };
    if (std::fread(fileContents.get(), 1, fileLength, fileIn) != std::size_t(fileLength))
    {
        warning("Failed to read whole shader file! \"" + fillPath + "\".");
        return nullptr;
    }

    fileContents[fileLength] = '\0';
    return fileContents;
}

const char * ShaderProgram::findShaderIncludes(const char * srcText, std::vector<std::string> & includeFiles)
{
    //
    // Very simple #include resolution. Include directives
    // should be the fist things in a shader file, besides comments.
    //

    std::string includeFile;
    const char * incPtr = std::strstr(srcText, "#include");

    for (; incPtr != nullptr; incPtr = std::strstr(incPtr, "#include"))
    {
        // Skip the directive:
        incPtr += std::strlen("#include");

        // Skip till first quote:
        for (; *incPtr != '\0' && *incPtr != '"'; ++incPtr) { }

        // Get the filename:
        if (*incPtr == '"')
        {
            ++incPtr; // Skip past the first quote
            for (; *incPtr != '\0' && *incPtr != '"'; ++incPtr)
            {
                includeFile.push_back(*incPtr);
            }

            includeFiles.emplace_back(std::move(includeFile));
            includeFile.clear();

            // Whatever text followed this include directive.
            srcText = ++incPtr;
        }
    }

    return srcText;
}

bool ShaderProgram::checkShaderInfoLogs(const UInt progHandle, const UInt vsHandle, const UInt fsHandle) noexcept
{
    constexpr int InfoLogMaxChars = 2048;

    GLsizei charsWritten;
    GLchar infoLogBuf[InfoLogMaxChars];

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetProgramInfoLog(progHandle, InfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warning("------ GL PROGRAM INFO LOG ----------");
        warning(infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(vsHandle, InfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warning("------ GL VERT SHADER INFO LOG ------");
        warning(infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(fsHandle, InfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warning("------ GL FRAG SHADER INFO LOG ------");
        warning(infoLogBuf);
    }

    GLint linkStatus = GL_FALSE;
    GLProxy::glGetProgramiv(progHandle, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE)
    {
        warning("Failed to link GL shader program!");
        return false;
    }

    return true;
}

void ShaderProgram::bind() const noexcept
{
    if (!isValid())
    {
        warning("Trying to bind an invalid shader program!");
        bindNull();
        return;
    }

    if (handle != currentProg)
    {
        currentProg = handle;
        GLProxy::glUseProgram(handle);
    }
}

void ShaderProgram::bindNull() noexcept
{
    currentProg = 0;
    GLProxy::glUseProgram(0);
}

const std::string & ShaderProgram::getGlslVersionDirective()
{
    // Queried once and stored for the subsequent shader loads.
    // This ensures we use the best version available.
    if (glslVersionDirective.empty())
    {
        int  slMajor    = 0;
        int  slMinor    = 0;
        int  versionNum = 0;
        auto versionStr = reinterpret_cast<const char *>(GLProxy::glGetString(GL_SHADING_LANGUAGE_VERSION));

        #ifdef _MSC_VER
        if (sscanf_s(versionStr, "%d.%d", &slMajor, &slMinor) == 2)
        #else // ! _MSC_VER
        if (std::sscanf(versionStr, "%d.%d", &slMajor, &slMinor) == 2)
        #endif // _MSC_VER
        {
            versionNum = (slMajor * 100) + slMinor;
        }
        else
        {
            // Fall back to the lowest acceptable version.
            // Assume #version 150 -> OpenGL 3.2
            versionNum = 150;
        }

        glslVersionDirective = "#version " + std::to_string(versionNum) + "\n";
    }

    return glslVersionDirective;
}

int ShaderProgram::getUniformLocation(const std::string & uniformName) const noexcept
{
    if (uniformName.empty() || !isValid())
    {
        return -1;
    }
    return GLProxy::glGetUniformLocation(handle, uniformName.c_str());
}

void ShaderProgram::setUniform1i(const int loc, const int x) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform1i: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform1i: Program not current!");
        return;
    }
    GLProxy::glUniform1i(loc, x);
}

void ShaderProgram::setUniform2i(const int loc, const int x, const int y) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform2i: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform2i: Program not current!");
        return;
    }
    GLProxy::glUniform2i(loc, x, y);
}

void ShaderProgram::setUniform3i(const int loc, const int x, const int y, const int z) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform3i: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform3i: Program not current!");
        return;
    }
    GLProxy::glUniform3i(loc, x, y, z);
}

void ShaderProgram::setUniform4i(const int loc, const int x, const int y, const int z, const int w) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform4i: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform4i: Program not current!");
        return;
    }
    GLProxy::glUniform4i(loc, x, y, z, w);
}

void ShaderProgram::setUniform1f(const int loc, const float x) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform1f: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform1f: Program not current!");
        return;
    }
    GLProxy::glUniform1f(loc, x);
}

void ShaderProgram::setUniform2f(const int loc, const float x, const float y) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform2f: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform2f: Program not current!");
        return;
    }
    GLProxy::glUniform2f(loc, x, y);
}

void ShaderProgram::setUniform3f(const int loc, const float x, const float y, const float z) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform3f: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform3f: Program not current!");
        return;
    }
    GLProxy::glUniform3f(loc, x, y, z);
}

void ShaderProgram::setUniform4f(const int loc, const float x, const float y, const float z, const float w) const noexcept
{
    if (loc < 0)
    {
        warning("setUniform4f: Invalid uniform location: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniform4f: Program not current!");
        return;
    }
    GLProxy::glUniform4f(loc, x, y, z, w);
}

void ShaderProgram::setUniformMat3(const int loc, const float * m) const noexcept
{
    if (loc < 0 || m == nullptr)
    {
        warning("setUniformMat3: Invalid uniform location or nullptr: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniformMat3: Program not current!");
        return;
    }
    GLProxy::glUniformMatrix3fv(loc, 1, GL_FALSE, m);
}

void ShaderProgram::setUniformMat4(const int loc, const float * m) const noexcept
{
    if (loc < 0 || m == nullptr)
    {
        warning("setUniformMat4: Invalid uniform location or nullptr: " + std::to_string(loc));
        return;
    }
    if (!isBound())
    {
        warning("setUniformMat4: Program not current!");
        return;
    }
    GLProxy::glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

// ========================================================
// class ShaderProgramManager:
// ========================================================

ShaderProgramManager * ShaderProgramManager::sharedInstance{ nullptr };

ShaderProgramManager::ShaderProgramManager()
{
    info("---- ShaderProgramManager startup ----");

    //
    // Warm up all the shaders we're going to need:
    //

    shaders[static_cast<int>(ShaderId::FramePostProcess)] =
        std::make_unique<ShaderProgram>("FramePostProcess.vert",
                                        "FramePostProcess.frag");
}

} // namespace War3 {}
