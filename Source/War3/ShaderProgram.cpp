
// ============================================================================
// File:   ShaderProgram.cpp
// Author: Guilherme R. Lampert
// Brief:  Shader Program management helpers.
// ============================================================================

#include "War3/ShaderProgram.hpp"
#include "GLProxy/GLExtensions.hpp"
#include <cstdio>  // For std::fopen/sscanf
#include <cstring> // For std::strstr

namespace War3
{

// ========================================================
// class ShaderProgram:
// ========================================================

unsigned ShaderProgram::sm_currentProg{};
std::string ShaderProgram::sm_glslVersionDirective{};

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_handle{ other.m_handle }
    , m_linkedOk{ other.m_linkedOk }
{
    other.invalidate();
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    releaseGLHandle();

    m_handle = other.m_handle;
    m_linkedOk = other.m_linkedOk;

    other.invalidate();
    return *this;
}

ShaderProgram::ShaderProgram(TextBuffer&& vsSrcText, TextBuffer&& fsSrcText, const std::string& directives)
    : m_handle{ 0 }
    , m_linkedOk{ false }
{
    GLProxy::loadInternalGLFunctions();

    if (vsSrcText == nullptr)
    {
        warn("Null Vertex Shader source!");
        return;
    }
    if (fsSrcText == nullptr)
    {
        warn("Null Fragment Shader source!");
        return;
    }

    //
    // Shader #include resolution:
    //

    std::string vsIncludedText;
    std::string fsIncludedText;
    std::vector<std::string> vsIncludes;
    std::vector<std::string> fsIncludes;

    const char* vsSrcTextPtr = findShaderIncludes(vsSrcText.get(), vsIncludes);
    const char* fsSrcTextPtr = findShaderIncludes(fsSrcText.get(), fsIncludes);

    for (const auto& incFile : vsIncludes)
    {
        info("Loading Vertex Shader include \"%s\"...", incFile.c_str());
        auto fileContents = loadShaderFile(incFile);
        vsIncludedText += (fileContents != nullptr ? fileContents.get() : "");
    }

    for (const auto& incFile : fsIncludes)
    {
        info("Loading Fragment Shader include \"%s\"...", incFile.c_str());
        auto fileContents = loadShaderFile(incFile);
        fsIncludedText += (fileContents != nullptr ? fileContents.get() : "");
    }

    //
    // GL handle allocation:
    //

    const auto glProgHandle = GLProxy::glCreateProgram();
    if (glProgHandle == 0)
    {
        warn("Failed to allocate a new GL Program handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
        return;
    }

    const auto glVsHandle = GLProxy::glCreateShader(GL_VERTEX_SHADER);
    const auto glFsHandle = GLProxy::glCreateShader(GL_FRAGMENT_SHADER);
    if (glVsHandle == 0 || glFsHandle == 0)
    {
        warn("Failed to allocate a new GL Shader handle! Possibly out-of-memory!");
        GLPROXY_CHECK_GL_ERRORS();
        return;
    }

    // Vertex shader:
    const char* vsSrcStrings[]{ getGlslVersionDirective().c_str(), directives.c_str(), vsIncludedText.c_str(), vsSrcTextPtr };
    GLProxy::glShaderSource(glVsHandle, arrayLength(vsSrcStrings), vsSrcStrings, nullptr);
    GLProxy::glCompileShader(glVsHandle);
    GLProxy::glAttachShader(glProgHandle, glVsHandle);

    // Fragment shader:
    const char* fsSrcStrings[]{ getGlslVersionDirective().c_str(), directives.c_str(), fsIncludedText.c_str(), fsSrcTextPtr };
    GLProxy::glShaderSource(glFsHandle, arrayLength(fsSrcStrings), fsSrcStrings, nullptr);
    GLProxy::glCompileShader(glFsHandle);
    GLProxy::glAttachShader(glProgHandle, glFsHandle);

    // Link the Shader Program then check and print the info logs, if any.
    GLProxy::glLinkProgram(glProgHandle);
    m_linkedOk = checkShaderInfoLogs(glProgHandle, glVsHandle, glFsHandle);

    // After a program is linked the shader objects can be safely detached and deleted.
    // Also recommended to save on the memory that would be wasted by keeping the shaders alive.
    GLProxy::glDetachShader(glProgHandle, glVsHandle);
    GLProxy::glDetachShader(glProgHandle, glFsHandle);
    GLProxy::glDeleteShader(glVsHandle);
    GLProxy::glDeleteShader(glFsHandle);

    // OpenGL likes to defer GPU resource allocation to the first time
    // an object is bound to the current state. Binding it now should
    // "warm up" the resource and avoid lag on the first frame rendered with it.
    sm_currentProg = glProgHandle;
    GLProxy::glUseProgram(glProgHandle);

    // Done, log errors and store the handle one way or the other.
    GLPROXY_CHECK_GL_ERRORS();
    m_handle = glProgHandle;
}

ShaderProgram::ShaderProgram(const std::string& vsFile, const std::string& fsFile, const std::string& directives)
    : ShaderProgram{ loadShaderFile(vsFile), loadShaderFile(fsFile), directives }
{
    if (m_handle != 0)
    {
        info("New ShaderProgram created from \"%s\" and \"%s\".", vsFile.c_str(), fsFile.c_str());
    }
}

ShaderProgram::~ShaderProgram()
{
    releaseGLHandle();
}

const char* ShaderProgram::getShaderPath() noexcept
{
    return "NewShaders\\";
}

void ShaderProgram::releaseGLHandle() noexcept
{
    if (m_handle != 0)
    {
        if (m_handle == sm_currentProg)
        {
            bindNull();
        }

        GLProxy::glDeleteProgram(m_handle);
        m_linkedOk = false;
        m_handle = 0;
    }
}

void ShaderProgram::invalidate() noexcept
{
    m_linkedOk = false;
    m_handle = 0;
}

ShaderProgram::TextBuffer ShaderProgram::loadShaderFile(const std::string& filename)
{
    if (filename.empty())
    {
        warn("Empty filename in ShaderProgram::loadShaderFile()!");
        return nullptr;
    }

    FILE* fileIn = nullptr;
    const auto fullPath = getShaderPath() + filename;
    fopen_s(&fileIn, fullPath.c_str(), "rb");

    if (fileIn == nullptr)
    {
        warn("Can't open shader file \"%s\"!", fullPath.c_str());
        return nullptr;
    }

    WAR3_SCOPE_EXIT(std::fclose(fileIn););

    std::fseek(fileIn, 0, SEEK_END);
    const auto fileLength = std::ftell(fileIn);
    std::fseek(fileIn, 0, SEEK_SET);

    if (fileLength <= 0 || std::ferror(fileIn))
    {
        warn("Error getting length or empty shader file! \"%s\".", fullPath.c_str());
        return nullptr;
    }

    TextBuffer fileContents{ new char[fileLength + 1] };
    if (std::fread(fileContents.get(), 1, fileLength, fileIn) != std::size_t(fileLength))
    {
        warn("Failed to read whole shader file! \"%s\".", fullPath.c_str());
        return nullptr;
    }

    fileContents[fileLength] = '\0';
    return fileContents;
}

const char* ShaderProgram::findShaderIncludes(const char* srcText, std::vector<std::string>& includeFiles)
{
    //
    // Very simple #include resolution. Include directives
    // should be the fist things in a shader file, besides comments.
    //

    std::string includeFile;
    const char* incPtr = std::strstr(srcText, "#include");

    for (; incPtr != nullptr; incPtr = std::strstr(incPtr, "#include"))
    {
        // Skip the directive:
        incPtr += std::strlen("#include");

        // Skip till first quote:
        for (; *incPtr != '\0' && *incPtr != '"'; ++incPtr)
        {
        }

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

bool ShaderProgram::checkShaderInfoLogs(const unsigned progHandle, const unsigned vsHandle, const unsigned fsHandle) noexcept
{
    constexpr int kInfoLogMaxChars = 2048;

    GLsizei charsWritten;
    GLchar infoLogBuf[kInfoLogMaxChars];

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetProgramInfoLog(progHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("------ GL PROGRAM INFO LOG ----------");
        warn("%s", infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(vsHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("------ GL VERT SHADER INFO LOG ------");
        warn("%s", infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(fsHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("------ GL FRAG SHADER INFO LOG ------");
        warn("%s", infoLogBuf);
    }

    GLint linkStatus = GL_FALSE;
    GLProxy::glGetProgramiv(progHandle, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE)
    {
        warn("Failed to link GL shader program!");
        return false;
    }

    return true;
}

void ShaderProgram::bind() const noexcept
{
    if (!isValid())
    {
        warn("Trying to bind an invalid shader program!");
        bindNull();
        return;
    }

    if (m_handle != sm_currentProg)
    {
        sm_currentProg = m_handle;
        GLProxy::glUseProgram(m_handle);
    }
}

void ShaderProgram::bindNull() noexcept
{
    sm_currentProg = 0;
    GLProxy::glUseProgram(0);
}

const std::string& ShaderProgram::getGlslVersionDirective()
{
    // Queried once and stored for the subsequent shader loads.
    // This ensures we use the best version available.
    if (sm_glslVersionDirective.empty())
    {
        int slMajor = 0;
        int slMinor = 0;
        int versionNum = 0;
        auto versionStr = reinterpret_cast<const char*>(GLProxy::glGetString(GL_SHADING_LANGUAGE_VERSION));

        if (sscanf_s(versionStr, "%d.%d", &slMajor, &slMinor) == 2)
        {
            versionNum = (slMajor * 100) + slMinor;
        }
        else
        {
            // Fall back to the lowest acceptable version.
            // Assume #version 150 -> OpenGL 3.2
            versionNum = 150;
        }

        sm_glslVersionDirective = "#version " + std::to_string(versionNum) + "\n";
    }

    return sm_glslVersionDirective;
}

int ShaderProgram::getUniformLocation(const std::string& uniformName) const noexcept
{
    if (uniformName.empty() || !isValid())
    {
        return -1;
    }
    return GLProxy::glGetUniformLocation(m_handle, uniformName.c_str());
}

void ShaderProgram::setUniform1i(const int loc, const int x) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform1i: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform1i: Program not current!");
        return;
    }
    GLProxy::glUniform1i(loc, x);
}

void ShaderProgram::setUniform2i(const int loc, const int x, const int y) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform2i: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform2i: Program not current!");
        return;
    }
    GLProxy::glUniform2i(loc, x, y);
}

void ShaderProgram::setUniform3i(const int loc, const int x, const int y, const int z) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform3i: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform3i: Program not current!");
        return;
    }
    GLProxy::glUniform3i(loc, x, y, z);
}

void ShaderProgram::setUniform4i(const int loc, const int x, const int y, const int z, const int w) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform4i: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform4i: Program not current!");
        return;
    }
    GLProxy::glUniform4i(loc, x, y, z, w);
}

void ShaderProgram::setUniform1f(const int loc, const float x) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform1f: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform1f: Program not current!");
        return;
    }
    GLProxy::glUniform1f(loc, x);
}

void ShaderProgram::setUniform2f(const int loc, const float x, const float y) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform2f: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform2f: Program not current!");
        return;
    }
    GLProxy::glUniform2f(loc, x, y);
}

void ShaderProgram::setUniform3f(const int loc, const float x, const float y, const float z) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform3f: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform3f: Program not current!");
        return;
    }
    GLProxy::glUniform3f(loc, x, y, z);
}

void ShaderProgram::setUniform4f(const int loc, const float x, const float y, const float z, const float w) const noexcept
{
    if (loc < 0)
    {
        warn("setUniform4f: Invalid uniform location: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniform4f: Program not current!");
        return;
    }
    GLProxy::glUniform4f(loc, x, y, z, w);
}

void ShaderProgram::setUniformMat3(const int loc, const float* m) const noexcept
{
    if (loc < 0 || m == nullptr)
    {
        warn("setUniformMat3: Invalid uniform location or nullptr: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniformMat3: Program not current!");
        return;
    }
    GLProxy::glUniformMatrix3fv(loc, 1, GL_FALSE, m);
}

void ShaderProgram::setUniformMat4(const int loc, const float* m) const noexcept
{
    if (loc < 0 || m == nullptr)
    {
        warn("setUniformMat4: Invalid uniform location or nullptr: %d", loc);
        return;
    }
    if (!isBound())
    {
        warn("setUniformMat4: Program not current!");
        return;
    }
    GLProxy::glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

// ========================================================
// class ShaderProgramManager:
// ========================================================

ShaderProgramManager* ShaderProgramManager::sm_sharedInstance{ nullptr };

ShaderProgramManager::ShaderProgramManager()
{
    info("---- ShaderProgramManager startup ----");

    // Warm up all the shaders we're going to need:
    //m_shaders[kFramePostProcess] = std::make_unique<ShaderProgram>("FramePostProcess.vert", "FramePostProcess.frag");
    //TODO - find another place for this!
}

} // namespace War3
