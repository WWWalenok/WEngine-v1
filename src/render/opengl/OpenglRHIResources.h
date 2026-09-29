#ifndef __OPENGLRHIRESOURCES_H__
#define __OPENGLRHIRESOURCES_H__

#include "../../core/core.h"
#include "../IRHIHelper.h"
#include "../IWindowHelper.h"
#include "../../graphic/Texture.h"
#include "../../graphic/Mesh.h"
#include "../../graphic/Image.h"
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Непрозрачные интерфейсы GPU‑ресурсов (CPU не может инстанциировать)
// ---------------------------------------------------------------------------
class IRHIShader : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHIShader)
};

class IRHIBuffer : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHIBuffer)
};

// ---------------------------------------------------------------------------
// OpenGL буфер (UBO / SSBO)
// ---------------------------------------------------------------------------
class OpenglBuffer : public IRHIBuffer
{
public:
    GLuint buffer = 0;
    GLenum target;
    size_t size = 0;
    void*  mapped = nullptr;   // persistent mapping

    OpenglBuffer(GLenum target = GL_UNIFORM_BUFFER) : target(target)
    {
        glGenBuffers(1, &buffer);
    }

    ~OpenglBuffer() override
    {
        if (buffer)
        {
            if (mapped)
            {
                glBindBuffer(target, buffer);
                glUnmapBuffer(target);
                mapped = nullptr;
            }
            glDeleteBuffers(1, &buffer);
        }
    }

    void Bind()   const { glBindBuffer(target, buffer); }
    void Unbind() const { glBindBuffer(target, 0); }
    void BindBase(GLuint index) const { glBindBufferBase(target, index, buffer); }

    // ----- Persistent mapped storage -----
    // Выделяет буфер и держит его замапленным. Возвращает true при успехе.
    bool InitPersistent(size_t bytes)
    {
        if (bytes == 0) return false;

        Bind();
        glBufferStorage(target, bytes, nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        mapped = glMapBufferRange(target, 0, bytes,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        if (!mapped) {
            // Откат: освободим буфер, чтобы старый путь работал.
            glDeleteBuffers(1, &buffer);
            glGenBuffers(1, &buffer);
            return false;
        }
        size = bytes;
        return true;
    }

    void* GetMapped() const { return mapped; }

    // ----- Fallback / обычный путь -----
    void SetData(const void* data, size_t dataSize)
    {
        if (mapped)
        {
            // Persistent: пишем напрямую в mapped-память.
            std::memcpy(mapped, data, dataSize);
            return;
        }
        Bind();
        if (dataSize > size)
        {
            glBufferData(target, dataSize, data, GL_DYNAMIC_DRAW);
            size = dataSize;
        }
        else
        {
            glBufferSubData(target, 0, dataSize, data);
        }
    }
};

// ---------------------------------------------------------------------------
// OpenGL шейдерная программа
// ---------------------------------------------------------------------------
class OpenglShader : public IRHIShader
{
public:
    GLuint program = 0;
    std::string vs_src, fs_src;
    OpenglShader() = default;

    OpenglShader(const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        vs_src = vertexSrc;
        fs_src = fragmentSrc;
    }

    void SetSrc(const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        vs_src = vertexSrc;
        fs_src = fragmentSrc;
    }

    void SetFS(const std::string &fragmentSrc)
    {
        fs_src = fragmentSrc;
    }

    void SetVS(const std::string &vertexSrc)
    {
        vs_src = vertexSrc;
    }

    bool Compile()
    {
        GLuint vs = CompileShader(GL_VERTEX_SHADER, vs_src.c_str());
        if (!vs)
            return false;
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fs_src.c_str());
        if (!fs)
        {
            glDeleteShader(vs);
            return false;
        }

        program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char info[512];
            glGetProgramInfoLog(program, 512, NULL, info);
            std::cerr << "Shader link error: " << info << std::endl;
            glDeleteProgram(program);
            program = 0;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        return program != 0;
    }

    void Use() const
    {
        glUseProgram(program);
    }

    GLint GetLoc(const char* name)
    {
        return glGetUniformLocation(program, name);
    }

    template<typename T>
    void setUniform(const char* name, const T& v)
    {
        GLint loc = glGetUniformLocation(program, name);
        setUniform(loc, v);
    }

    void setUniform(GLint loc, const int& f)
    {
        if (loc != -1)
        {
            glUniform1i(loc, f);
        }
    }

    void setUniform(GLint loc, const float& f)
    {
        if (loc != -1)
        {
            glUniform1f(loc, f);
        }
    }

    void setUniform(GLint loc, const MVector2f& vec)
    {
        if (loc != -1)
        {
            glUniform2f(loc, vec[0], vec[1]);
        }
    }

    void setUniform(GLint loc, const MVector3f& vec)
    {
        if (loc != -1)
        {
            glUniform3f(loc, vec[0], vec[1], vec[2]);
        }
    }

    void setUniform(GLint loc, const MVector4f& vec)
    {
        if (loc != -1)
        {
            glUniform4f(loc, vec[0], vec[1], vec[2], vec[3]);
        }
    }

    void setUniform(GLint loc, const MMatrix2f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix2fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }

    void setUniform(GLint loc, const MMatrix3f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix3fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }

    void setUniform(GLint loc, const MMatrix4f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix4fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }

    void SetUniformBlockBinding(const char *name, GLuint binding)
    {
        GLuint index = glGetUniformBlockIndex(program, name);
        if (index != GL_INVALID_INDEX)
            glUniformBlockBinding(program, index, binding);
    }

private:
    static GLuint CompileShader(GLenum type, const std::string &src)
    {
        GLuint shader = glCreateShader(type);
        const char *cstr = src.c_str();
        glShaderSource(shader, 1, &cstr, NULL);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, NULL, info);
            std::cerr << "Shader " << type << " compile error: " << info << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
};

// ---------------------------------------------------------------------------
// Конвертеры PixelFormat -> OpenGL
// ---------------------------------------------------------------------------
GLenum PixelFormat2GLFormat(const PixelFormat &m)
{
    switch (m)
    {
        case PixelFormat::RED_8:
        case PixelFormat::RED_F:
            return GL_RED;
        case PixelFormat::RG_8:
        case PixelFormat::RG_F:
            return GL_RG;
        case PixelFormat::RGB_8:
        case PixelFormat::RGB_F:
            return GL_RGB;
        case PixelFormat::RGBA_8:
        case PixelFormat::RGBA_F:
            return GL_RGBA;
        case PixelFormat::RED_16:
        case PixelFormat::RED_32:
            return GL_RED_INTEGER;
        case PixelFormat::RG_16:
        case PixelFormat::RG_32:
            return GL_RG_INTEGER;
        case PixelFormat::RGB_16:
        case PixelFormat::RGB_32:
            return GL_RGB_INTEGER;
        case PixelFormat::RGBA_16:
        case PixelFormat::RGBA_32:
            return GL_RGBA_INTEGER;
        default:
            return 0;
    }
}

GLenum PixelFormat2GLInternal(const PixelFormat &m)
{
    switch (m)
    {
        case PixelFormat::RED_8:
            return GL_R8;
        case PixelFormat::RED_16:
            return GL_R16UI;
        case PixelFormat::RED_32:
            return GL_R32UI;
        case PixelFormat::RED_F:
            return GL_R32F;
        case PixelFormat::RG_8:
            return GL_RG8;
        case PixelFormat::RG_16:
            return GL_RG16UI;
        case PixelFormat::RG_32:
            return GL_RG32UI;
        case PixelFormat::RG_F:
            return GL_RG32F;
        case PixelFormat::RGB_8:
            return GL_RGB8;
        case PixelFormat::RGB_16:
            return GL_RGB16UI;
        case PixelFormat::RGB_32:
            return GL_RGB32UI;
        case PixelFormat::RGB_F:
            return GL_RGB32F;
        case PixelFormat::RGBA_8:
            return GL_RGBA8;
        case PixelFormat::RGBA_16:
            return GL_RGBA16UI;
        case PixelFormat::RGBA_32:
            return GL_RGBA32UI;
        case PixelFormat::RGBA_F:
            return GL_RGBA32F;
        default:
            return 0;
    }
}

GLenum PixelFormat2GLType(const PixelFormat &m)
{
    switch (m)
    {
        case PixelFormat::RED_8:
        case PixelFormat::RG_8:
        case PixelFormat::RGB_8:
        case PixelFormat::RGBA_8:
            return GL_UNSIGNED_BYTE;
        case PixelFormat::RED_16:
        case PixelFormat::RG_16:
        case PixelFormat::RGB_16:
        case PixelFormat::RGBA_16:
            return GL_UNSIGNED_SHORT;
        case PixelFormat::RED_32:
        case PixelFormat::RG_32:
        case PixelFormat::RGB_32:
        case PixelFormat::RGBA_32:
            return GL_UNSIGNED_INT;
        case PixelFormat::RED_F:
        case PixelFormat::RG_F:
        case PixelFormat::RGB_F:
        case PixelFormat::RGBA_F:
            return GL_FLOAT;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// Текстура с поддержкой использования как Render Target
// ---------------------------------------------------------------------------
class OpenglRHITexture : public IRHITexture
{
public:
    GLuint Tex = 0;
    GLuint FBO = 0;
    GLuint RBO = 0;
    PixelFormat mt;
    int width = 0, height = 0, depth = 1;
    int rbo_width = 0, rbo_height = 0; // <-- Кэшируем размеры RBO

    void Update(Texture *ref_tex) override
    {
        Ref<Image> image = ref_tex->image;
        if (!image)
            return;
        mt = image->format;
        width = image->width;
        height = image->height;
        if (!Tex)
            glGenTextures(1, &Tex);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glTexImage2D(GL_TEXTURE_2D,
          0,
          PixelFormat2GLInternal(mt),
          width,
          height,
          0,
          PixelFormat2GLFormat(mt),
          PixelFormat2GLType(mt),
          image->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void Update(int w, int h, PixelFormat format, const void* data) {
        width = w; height = h; mt = format;
        if (!Tex) glGenTextures(1, &Tex);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glTexImage2D(GL_TEXTURE_2D, 0, PixelFormat2GLInternal(format), w, h, 0,
                    PixelFormat2GLFormat(format), PixelFormat2GLType(format), data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        EnsureFBO();
    }

    void InitAsRenderTarget(int w, int h, PixelFormat format, const void* data = nullptr, bool useFilter = true)
    {
        width = w;
        height = h;
        mt = format;
        if (!Tex)
            glGenTextures(1, &Tex);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glTexImage2D(GL_TEXTURE_2D,
          0,
          PixelFormat2GLInternal(format),
          w,
          h,
          0,
          PixelFormat2GLFormat(format),
          PixelFormat2GLType(format),
          data);
        if(useFilter)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        EnsureFBO();
    }

    void EnsureFBO()
    {
        if (!FBO)
        {
            glGenFramebuffers(1, &FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Tex, 0);
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Framebuffer incomplete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        if (!RBO || rbo_width != width || rbo_height != height)
        {
            if (!RBO)
                glGenRenderbuffers(1, &RBO);

            glBindFramebuffer(GL_FRAMEBUFFER, FBO);

            glBindRenderbuffer(GL_RENDERBUFFER, RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Tex, 0);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Framebuffer incomplete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            
            rbo_width = width;
            rbo_height = height;
        }
    }

    void BindAsTarget()
    {
        EnsureFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, width, height);
    }

    void UnbindTarget()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GetSize(int &w, int &h, int &d) const override
    {
        w = width;
        h = height;
        d = depth;
    }

    PixelFormat GetPixelFormat() const override
    {
        return mt;
    }

    ~OpenglRHITexture() override
    {
        if (Tex)
            glDeleteTextures(1, &Tex);
        if (FBO)
            glDeleteFramebuffers(1, &FBO);
    }
};

class OpenglRHISkeleton : public IRHISkeleton
{
public:
    GLuint SSBO = 0;
    Skeleton *skeleton;
    bool need_update;

    void Update(Skeleton *skeleton) override
    {
        if (this->skeleton != skeleton)
        {
            this->skeleton = skeleton;
        }
        if (!glGenVertexArrays || !GetIRHIHelper()->Inited())
        {
            need_update = true;
            return;
        }
        if (!SSBO)
            glGenBuffers(1, &SSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
          skeleton->boneMatrices.size() * sizeof(skeleton->boneMatrices[0]),
          skeleton->boneMatrices.data(),
          GL_DYNAMIC_DRAW);
    }
};

class OpenglRHIMesh : public IRHIMesh
{
public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;
    bool need_update = true;

    void Update(Mesh *mesh) override
    {
        if (!glGenVertexArrays || !GetIRHIHelper()->Inited())
        {
            need_update = true;
            return;
        }
        indexCount = (GLsizei)mesh->f.size();
        if (!VAO)
            glGenVertexArrays(1, &VAO);
        if (!VBO)
            glGenBuffers(1, &VBO);
        if (!EBO)
            glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh->v.size() * sizeof(Mesh::Vertex), mesh->v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->f.size() * sizeof(uint32_t), mesh->f.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void *)offsetof(Mesh::Vertex, v));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void *)offsetof(Mesh::Vertex, vt));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void *)offsetof(Mesh::Vertex, vn));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        need_update = false;
    }

    void Update(SkeletalMesh *mesh) override
    {
        // TODO: реализация для скелетного меша
    }

    void Draw()
    {
        if (need_update)
            return;
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void DrawInstanced(GLsizei instanceCount)
    {
        if (need_update || instanceCount <= 0)
            return;
        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount,
                                GL_UNSIGNED_INT, 0, instanceCount);
    }
};

class OpenglRHIWindowTarget : public IRHIWindowTarget
{
    IWindow *window; // храним окно, которому делегируем вызовы

public:
    void Update(IWindow *w) override
    {
        window = w;
    }

    void Activate() override
    {
        if (window)
            window->Activate();
    }
    void Swap() override
    {
        if (window)
            window->Swap();
    }
    void GetSize(int &w, int &h) const override
    {
        if (window)
        {
            window->GetSize(w, h);
        }
        else
        {
            w = 0;
            h = 0;
        }
    }
    virtual bool Valid() const {
        return window && window->Valid();
    }
};

class MeshBatchResource : public IRHIResource
{
public:
    RHI_TYPE_IMP(MeshBatchResource)

    Ref<OpenglRHIMesh>    mesh;
    Ref<OpenglRHITexture> diffuse;
    Ref<OpenglRHITexture> specular;
};

class InstanceDataResource : public IRHIResource
{
public:
    RHI_TYPE_IMP(InstanceDataResource)

    std::vector<MMatrix4f> transforms;

    Ref<OpenglBuffer> buffer;

    MMatrix4f* gpu_data = nullptr;
    size_t     count    = 0;

    InstanceDataResource(size_t reserve = 0)
    {
        buffer = MakeRef<OpenglBuffer>(GL_SHADER_STORAGE_BUFFER);

        if (reserve > 0)
        {
            const size_t bytes = reserve * sizeof(MMatrix4f);
            if (reserve > 128 && buffer->InitPersistent(bytes))
            {
                gpu_data = static_cast<MMatrix4f*>(buffer->GetMapped());
                count    = reserve;

                // Инициализируем слоты identity — WObject потом перезапишет.
                for (size_t i = 0; i < reserve; ++i)
                    gpu_data[i] = MMatrix4f::identity();
            }
            else
            {
                // Fallback: старая схема.
                transforms.resize(reserve, MMatrix4f::identity());
                count = reserve;
            }
        }
    }

    MMatrix4f* Data()
    {
        return gpu_data ? gpu_data : transforms.data();
    }

    void Upload()
    {
        if (gpu_data)
        {
            return;
        }
        count = transforms.size();
        if (count == 0) return;
        buffer->SetData(transforms.data(), count * sizeof(MMatrix4f));
    }
};

#endif // __OPENGLRHIRESOURCES_H__