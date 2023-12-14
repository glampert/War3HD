
// ============================================================================
// File:   ShaderProgram.cpp
// Author: Guilherme R. Lampert
// Brief:  Shader Program management helpers.
// ============================================================================

#include "ShaderProgram.hpp"
#include "GLProxy/GLExtensions.hpp"
#include <cstdio>  // For std::fopen/sscanf
#include <cstring> // For std::strstr

// Always use this version if this macro is defined.
//
// Some built-ins like gl_Vertex, gl_TexCoord, etc we use in the shaders
// were deprecated and are no longer supported on newer OpenGL drivers.
//
#define WAR3_FORCE_GLSL_VERSION 130

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

ShaderProgram::ShaderProgram(TextBuffer&& vsSrcText, TextBuffer&& fsSrcText, TextBuffer&& gsSrcText,
                             const std::string& directives, const std::initializer_list<std::string>& optDebugFileNames)
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
    // gsSrcText is optional.

    //
    // Shader #include resolution:
    //

    std::string vsIncludedText;
    std::string fsIncludedText;
    std::string gsIncludedText;

    std::vector<std::string> vsIncludes;
    std::vector<std::string> fsIncludes;
    std::vector<std::string> gsIncludes;

    const char* vsSrcTextPtr = findShaderIncludes(vsSrcText.get(), vsIncludes);
    const char* fsSrcTextPtr = findShaderIncludes(fsSrcText.get(), fsIncludes);
    const char* gsSrcTextPtr = findShaderIncludes(gsSrcText.get(), gsIncludes);

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

    for (const auto& incFile : gsIncludes)
    {
        info("Loading Geometry Shader include \"%s\"...", incFile.c_str());
        auto fileContents = loadShaderFile(incFile);
        gsIncludedText += (fileContents != nullptr ? fileContents.get() : "");
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

    // Optional Geometry shader:
    GLuint glGsHandle = 0;
    if (gsSrcText != nullptr)
    {
        glGsHandle = GLProxy::glCreateShader(GL_GEOMETRY_SHADER);
        if (glGsHandle == 0)
        {
            warn("Failed to allocate a new GL Geometry Shader handle! Possibly out-of-memory!");
            GLPROXY_CHECK_GL_ERRORS();
            return;
        }
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

    // Optional Geometry shader:
    if (glGsHandle != 0)
    {
        const char* gsSrcStrings[]{ getGlslVersionDirective().c_str(), directives.c_str(), gsIncludedText.c_str(), gsSrcTextPtr };
        GLProxy::glShaderSource(glGsHandle, arrayLength(gsSrcStrings), gsSrcStrings, nullptr);
        GLProxy::glCompileShader(glGsHandle);
        GLProxy::glAttachShader(glProgHandle, glGsHandle);

        // These are the GL defaults.
        GLProxy::glProgramParameteri(glProgHandle, GL_GEOMETRY_INPUT_TYPE, GL_TRIANGLES);
        GLProxy::glProgramParameteri(glProgHandle, GL_GEOMETRY_OUTPUT_TYPE, GL_TRIANGLE_STRIP);

        // Necessary, otherwise the Geometry shader behaves weirdly...
        GLProxy::glProgramParameteri(glProgHandle, GL_GEOMETRY_VERTICES_OUT, 3);
        GLProxy::checkGLErrors(__FUNCTION__, __FILE__, __LINE__, /*crash=*/true);
    }

    // Link the Shader Program then check and print the info logs, if any.
    GLProxy::glLinkProgram(glProgHandle);
    m_linkedOk = checkShaderInfoLogs(glProgHandle, glVsHandle, glFsHandle, glGsHandle, optDebugFileNames);

    // After a program is linked the shader objects can be safely detached and deleted.
    // Also recommended to save on the memory that would be wasted by keeping the shaders alive.
    GLProxy::glDetachShader(glProgHandle, glVsHandle);
    GLProxy::glDetachShader(glProgHandle, glFsHandle);
    GLProxy::glDeleteShader(glVsHandle);
    GLProxy::glDeleteShader(glFsHandle);

    if (glGsHandle != 0)
    {
        GLProxy::glDetachShader(glProgHandle, glGsHandle);
        GLProxy::glDeleteShader(glGsHandle);
    }

    // OpenGL likes to defer GPU resource allocation to the first time
    // an object is bound to the current state. Binding it now should
    // "warm up" the resource and avoid lag on the first frame rendered with it.
    sm_currentProg = glProgHandle;
    GLProxy::glUseProgram(glProgHandle);

    // Done, log errors and store the handle one way or the other.
    GLPROXY_CHECK_GL_ERRORS();
    m_handle = glProgHandle;
}

ShaderProgram::ShaderProgram(const std::string& vsFile, const std::string& fsFile, const std::string& gsFile, const std::string& directives)
    : ShaderProgram{ loadShaderFile(vsFile), loadShaderFile(fsFile), loadShaderFile(gsFile), directives, { vsFile, fsFile, gsFile } }
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

    if (srcText == nullptr || *srcText == '\0')
    {
        return "";
    }

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

bool ShaderProgram::checkShaderInfoLogs(const unsigned progHandle, const unsigned vsHandle, const unsigned fsHandle, const unsigned gsHandle,
                                        const std::initializer_list<std::string>& optDebugFileNames) noexcept
{
    const char* vsFileName = "";
    const char* fsFileName = "";
    const char* gsFileName = "";

    if (optDebugFileNames.size() >= 3)
    {
        vsFileName = optDebugFileNames.begin()[0].c_str();
        fsFileName = optDebugFileNames.begin()[1].c_str();
        gsFileName = optDebugFileNames.begin()[2].c_str();
    }

    constexpr int kInfoLogMaxChars = 2048;

    GLsizei charsWritten;
    GLchar infoLogBuf[kInfoLogMaxChars];

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetProgramInfoLog(progHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("");
        warn("------ GL PROGRAM INFO LOG ----------");
        warn("[ %s, %s, %s ]", vsFileName, fsFileName, gsFileName);
        warn("%s", infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(vsHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("------ GL VERT SHADER INFO LOG ------");
        warn("[ %s ]", vsFileName);
        warn("%s", infoLogBuf);
    }

    charsWritten = 0;
    std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
    GLProxy::glGetShaderInfoLog(fsHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
    if (charsWritten > 0)
    {
        warn("------ GL FRAG SHADER INFO LOG ------");
        warn("[ %s ]", fsFileName);
        warn("%s", infoLogBuf);
    }

    // Geometry shader is optional.
    if (gsHandle != 0)
    {
        charsWritten = 0;
        std::memset(infoLogBuf, 0, sizeof(infoLogBuf));
        GLProxy::glGetShaderInfoLog(gsHandle, kInfoLogMaxChars - 1, &charsWritten, infoLogBuf);
        if (charsWritten > 0)
        {
            warn("------ GL GEOM SHADER INFO LOG ------");
            warn("[ %s ]", gsFileName);
            warn("%s", infoLogBuf);
        }  
    }

    GLint linkStatus = GL_FALSE;
    GLProxy::glGetProgramiv(progHandle, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE)
    {
        fatalError("Failed to link GL shader program: %s, %s, %s", vsFileName, fsFileName, gsFileName);
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
        int versionNum = 0;

        #if !defined(WAR3_FORCE_GLSL_VERSION)
        {
            int slMajor = 0;
            int slMinor = 0;
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
        }
        #else // WAR3_FORCE_GLSL_VERSION
        {
            versionNum = WAR3_FORCE_GLSL_VERSION;
        }
        #endif // WAR3_FORCE_GLSL_VERSION

        sm_glslVersionDirective = "#version " + std::to_string(versionNum) + "\n";

        info("GLSL version: %s", sm_glslVersionDirective.c_str());
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

void ShaderProgram::setProgramParameter(const unsigned paramId, const int value) const noexcept
{
    if (!isBound() || !isValid())
    {
        warn("setProgramParameter: Program not current!");
        return;
    }
    GLProxy::glProgramParameteri(m_handle, paramId, value);
}

void ShaderProgram::setGeometryInputType(const int type) const noexcept
{
    setProgramParameter(GL_GEOMETRY_INPUT_TYPE, type);
    GLPROXY_CHECK_GL_ERRORS();
}

void ShaderProgram::setGeometryOutputType(const int type) const noexcept
{
    setProgramParameter(GL_GEOMETRY_OUTPUT_TYPE, type);
    GLPROXY_CHECK_GL_ERRORS();
}

void ShaderProgram::setGeometryOutputVertexCount(const unsigned count) const noexcept
{
    setProgramParameter(GL_GEOMETRY_VERTICES_OUT, count);
    GLPROXY_CHECK_GL_ERRORS();
}

// ========================================================
// class ShaderProgramFactory:
// ========================================================

struct ShaderProgramManager::ShaderProgramFactory final
{
    ShaderProgramManager& m_spm;

    template<typename T>
    void CreateShaderVertFragGeom(const ShaderId shaderId, const char* const vertexShaderFile, const char* const fragmentShaderFile, const char* const geometryShaderFile)
    {
        static_assert(std::is_base_of_v<ShaderProgram, T>, "Class must inherit from ShaderProgram.");

        m_spm.m_shaders[shaderId] = std::make_unique<T>(vertexShaderFile, fragmentShaderFile, geometryShaderFile);
        if (!m_spm.m_shaders[shaderId]->isValid())
        {
            fatalError("Failed to create shader: '%s' - '%s' - '%s'", vertexShaderFile, fragmentShaderFile, geometryShaderFile);
        }
    }

    template<typename T>
    void CreateShaderPostProcess(const ShaderId shaderId, const char* const fragmentShaderFile)
    {
        static_assert(std::is_base_of_v<PostProcessShaderProgram, T>, "Class must inherit from PostProcessShaderProgram.");

        m_spm.m_shaders[shaderId] = std::make_unique<T>(fragmentShaderFile);
        if (!m_spm.m_shaders[shaderId]->isValid())
        {
            fatalError("Failed to create post-process shader: '%s'", fragmentShaderFile);
        }
    }
};

// ========================================================
// class ShaderProgramManager:
// ========================================================

ShaderProgramManager* ShaderProgramManager::sm_sharedInstance{ nullptr };

ShaderProgramManager::ShaderProgramManager()
{
    info("---- ShaderProgramManager startup ----");

    // Load all the shaders we're going to need:

    ShaderProgramFactory factory{ *this };

    factory.CreateShaderPostProcess<PostProcessShaderProgram>(kPresentFramebuffer, "PresentFramebuffer.frag");
    factory.CreateShaderPostProcess<PostProcessShaderProgram>(kFramePostProcess, "FramePostProcess.frag");
    factory.CreateShaderPostProcess<FXAADebugShaderProgram>(kFXAADebug, "FXAA.frag");
    factory.CreateShaderVertFragGeom<DebugShaderProgram>(kDebug, "Debug.vert", "Debug.frag", "Debug.geom");
}

} // namespace War3
