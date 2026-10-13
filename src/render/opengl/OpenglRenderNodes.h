#ifndef __OPENGLRENDERNODES_H__
#define __OPENGLRENDERNODES_H__

#include "../../core/core.h"
#include "OpenglRHIResources.h"
#include "OpenglShaderLib.h"
#include <unordered_set>

class OpenglRenderGraph;
class OpenglRenderGraphBuilder;

struct OpenglContext
{
    bool rebind;
    Ref<OpenglRenderGraphBuilder> RGB;
    int width;
    int height;
};

class OpenglRHINode
{
public:
    virtual ~OpenglRHINode() = default;

    void SetInputNodesCount(size_t count)
    {
        inputNodes.resize(count);
    }
    std::vector<Ref<OpenglRHINode>> GetInputs() const
    {
        return inputNodes;
    }
    void SetInputNode(int idx, Ref<OpenglRHINode> node)
    {
        if (idx >= 0 && idx < (int)inputNodes.size())
            inputNodes[idx] = node;
    }

    void AddRequirement(Ref<OpenglRHINode> node)
    {
        requirementNodes.insert(node);
    }
    void RemoveRequirement(Ref<OpenglRHINode> node)
    {
        requirementNodes.erase(node);
    }

    bool CanExecute() const
    {
        for (auto& req : requirementNodes)
        {
            if (req && !req->IsProcessed())
                return false;
        }
        for (auto& in : inputNodes)
        {
            if (in && !in->IsProcessed())
                return false;
        }
        return true;
    }

    virtual void ResolveOutput()
    {
        if (HasOwnOutput() || !parentNode)
            return;
        auto select = parentNode;
        do
        {
            if (select->HasOwnOutput())
            {
                SetOutput(select->GetOutput());
                return;
            }
            select = select->GetParent();
        } while (select);
    }

    virtual void Prepair(OpenglContext& context) {};
    virtual void Execute(const OpenglContext& context) = 0;
    virtual bool HasOwnOutput() const
    {
        return false;
    }
    Ref<IRHIResource> GetOutput() const
    {
        return cashedOutput;
    }
    void SetOutput(Ref<IRHIResource> res)
    {
        if (!HasOwnOutput())
            cashedOutput = res;
    }
    bool IsProcessed() const
    {
        return processed;
    }
    Ref<OpenglRHINode> GetParent()
    {
        return parentNode;
    }
    void SetParent(Ref<OpenglRHINode> parent)
    {
        parentNode = parent;
    }
    virtual void ProcessOutputList(std::list<Ref<OpenglRHINode>>){};

protected:
    void MarkProcessed()
    {
        processed = true;
    }
    void ResetProcessed()
    {
        processed = false;
    }

    Ref<OpenglRHINode> parentNode;
    std::vector<Ref<OpenglRHINode>> inputNodes;
    std::unordered_set<Ref<OpenglRHINode>> requirementNodes;
    Ref<IRHIResource> cashedOutput;
    bool processed = false;

    friend class OpenglRenderGraph;
};

class OpenglTextureResourceNode : public OpenglRHINode
{
protected:
    Ref<OpenglRHITexture> texture;

public:
    OpenglTextureResourceNode(Ref<IRHITexture> tex) : texture(RefCast<OpenglRHITexture>(tex))
    {
        cashedOutput = texture;
    }
    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

class ColorTextureResourceNode : public OpenglTextureResourceNode
{
public:
    ColorTextureResourceNode(MVector3f color) : OpenglTextureResourceNode(MakeRef<OpenglRHITexture>())
    {
        uint8_t data[4] = {(uint8_t)(color[0] * 255), (uint8_t)(color[1] * 255), (uint8_t)(color[2] * 255), 255};
        texture->InitAsRenderTarget(1, 1, RGBA_8, data, false);
        cashedOutput = texture;
    }
};

class NullTextureResourceNode : public OpenglTextureResourceNode
{
public:
    static Ref<NullTextureResourceNode> Get()
    {
        static Ref<NullTextureResourceNode> _ = MakeRef<NullTextureResourceNode>();
        return _;
    }

    NullTextureResourceNode() : OpenglTextureResourceNode(MakeRef<OpenglRHITexture>())
    {
        static uint32_t data[4] = {0x00ff00ff, 0x00000000, 0x00000000, 0x00ff00ff};
        texture->InitAsRenderTarget(2, 2, RGBA_8, data, false);
        cashedOutput = texture;
    }
};

class OpenglImageResourceNode : public OpenglRHINode
{
    WeakRef<Image> imageRef;
    Ref<OpenglRHITexture> texture;

public:
    OpenglImageResourceNode(Ref<Image> img) : imageRef(img)
    {
        texture = MakeRef<OpenglRHITexture>();
        if (img)
        {
            uint32_t w = img->width;
            uint32_t h = img->height;
            PixelFormat fmt = img->format;
            const uint8_t* dataPtr = img->data;
            if (w > 0 && h > 0 && dataPtr)
            {
                texture->InitAsRenderTarget(w, h, fmt, dataPtr);
            }
            else
            {
                static uint8_t white[4] = {255, 255, 255, 255};
                texture->InitAsRenderTarget(1, 1, RGBA_8, white);
            }
        }
        cashedOutput = texture;
    }

    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto img = imageRef.lock();
        if (!img)
            return;
        if (img->dirty)
        {
            uint32_t w = img->width;
            uint32_t h = img->height;
            PixelFormat fmt = img->format;
            const uint8_t* dataPtr = img->data;
            if (w > 0 && h > 0 && dataPtr)
            {
                texture->Update(w, h, fmt, dataPtr);
            }
            img->Update();
        }
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }

    bool HasOwnOutput() const override
    {
        return true;
    }
};

class MeshRecourceNode : public OpenglRHINode
{
protected:
    WeakRef<Mesh> meshRef;
    Ref<IRHIMesh> mesh;

public:
    MeshRecourceNode(Ref<Mesh> inMesh) : meshRef(inMesh)
    {
        mesh = inMesh->rhimesh;
        cashedOutput = mesh;
    }
    MeshRecourceNode(Ref<IRHIMesh> inMesh) : mesh(inMesh), meshRef(nullptr)
    {;
        cashedOutput = mesh;
    }
    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto localRef = meshRef.lock();
        if (localRef)
        {
            localRef->Update();
        }
    }
    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

#include "../../World/WObject.h"
#include "../../World/Camera.h"

class WObjectTransformNode : public OpenglRHINode
{
protected:
    WeakRef<WObject> wobjectRef;
    Ref<TrivialRHIResource<MMatrix4f>> transform;

public:
    WObjectTransformNode(Ref<WObject> inwobjectRef) : wobjectRef(inwobjectRef)
    {
        transform = MakeRef<TrivialRHIResource<MMatrix4f>>(inwobjectRef->GetWorldTransform());
        cashedOutput = transform;
    }
    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto localRef = wobjectRef.lock();
        if (localRef)
        {
            auto tr = localRef->GetWorldTransform();
            transform->Set(tr);
        }
    }
    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

class CameraViewMatrixNode : public OpenglRHINode
{
protected:
    WeakRef<Camera> wRef;
    Ref<TrivialRHIResource<MMatrix4f>> wm;

public:
    CameraViewMatrixNode(Ref<Camera> inwRef) : wRef(inwRef)
    {
        wm = MakeRef<TrivialRHIResource<MMatrix4f>>(inwRef->GetViewMatrix());
        cashedOutput = wm;
    }
    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto localRef = wRef.lock();
        if (localRef)
        {
            wm->Set(localRef->GetViewMatrix());
        }
    }
    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

class CameraProjectionMatrixNode : public OpenglRHINode
{
    WeakRef<Camera> cameraRef;
    float aspect;
    Ref<TrivialRHIResource<MMatrix4f>> proj;

public:
    CameraProjectionMatrixNode(Ref<Camera> cam, float aspectRatio) : cameraRef(cam), aspect(aspectRatio)
    {
        proj = MakeRef<TrivialRHIResource<MMatrix4f>>(cam->GetProjectionMatrix(aspectRatio));
        cashedOutput = proj;
    }
    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto cam = cameraRef.lock();
        if (cam)
            proj->Set(cam->GetProjectionMatrix(aspect));
    }
    void Execute(const OpenglContext& context) override
    {
        aspect = context.width / (float)(context.height);
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

class ViewPosNode : public OpenglRHINode
{
    WeakRef<Camera> cameraRef;
    Ref<TrivialRHIResource<MVector3f>> viewPos;

public:
    ViewPosNode(Ref<Camera> cam) : cameraRef(cam)
    {
        viewPos = MakeRef<TrivialRHIResource<MVector3f>>(cam->GetWorldPosition());
        cashedOutput = viewPos;
    }
    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto cam = cameraRef.lock();
        if (cam)
            viewPos->Set(cam->GetWorldPosition());
    }
    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return true;
    }
};

class FullscreenQuadHelper
{
public:
    static GLuint GetVAO()
    {
        static GLuint quadVAO = 0;
        static bool initialized = false;
        if (!initialized)
        {
            float vertices[] = {-1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f};
            GLuint VBO;
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glBindVertexArray(0);
            glDeleteBuffers(1, &VBO);
            initialized = true;
        }
        return quadVAO;
    }

    static GLuint GetVAOIndex()
    {
        static GLuint quadVAO = 0;
        static bool initialized = false;
        if (!initialized)
        {
            float vertices[] = {
              // позиция (x,y)   текстурные координаты (u,v)
              -1.0f,
              1.0f,
              0.0f,
              1.0f, // левый верхний
              -1.0f,
              -1.0f,
              0.0f,
              0.0f, // левый нижний
              1.0f,
              -1.0f,
              1.0f,
              0.0f, // правый нижний
              1.0f,
              1.0f,
              1.0f,
              1.0f // правый верхний
            };
            unsigned int indices[] = {
              0,
              1,
              2, // первый треугольник
              0,
              2,
              3 // второй треугольник
            };

            GLuint VBO, EBO;
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(quadVAO);

            // Вершинный буфер
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // Индексный буфер
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // Атрибут 0: позиция
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            // Атрибут 1: текстурные координаты
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            glBindVertexArray(0);
            // VBO и EBO можно не удалять, они будут жить пока жив VAO
            initialized = true;
        }
        return quadVAO;
    }
};

class OpenglSequenceNode : public OpenglRHINode
{
public:
    OpenglSequenceNode()
    {
    }

    void AddSequenceNode(Ref<OpenglRHINode> node)
    {
        if (node)
            inputNodes.push_back(node);
    }

    void Execute(const OpenglContext& context) override
    {
        for (auto& child : inputNodes)
        {
            if (!child)
                continue;
            if (!child->CanExecute() || !child->IsProcessed())
            {
                return;
            }
        }
        MarkProcessed();
    }
};

class OpenglClearNode : public OpenglRHINode
{
    float clearColor[4];

public:
    OpenglClearNode(const float color[4])
    {
        memcpy(clearColor, color, sizeof(clearColor));
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        auto tex = RefCast<OpenglRHITexture>(cashedOutput);
        if (!tex)
        {
            MarkProcessed();
            return;
        }
        tex->BindAsTarget();
        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        tex->UnbindTarget();
        MarkProcessed();
    }
};

class OpenglFullscreenQuadNode : public OpenglRHINode
{
    Ref<OpenglShader> shader;

public:
    OpenglFullscreenQuadNode(Ref<IRHIShader> sh, size_t inputCount) : shader(RefCast<OpenglShader>(sh))
    {
        SetInputNodesCount(inputCount);
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        if (!shader)
        {
            MarkProcessed();
            return;
        }
        std::vector<Ref<OpenglRHITexture>> textures;
        for (auto& in : inputNodes)
        {
            if (!in)
            {
                MarkProcessed();
                return;
            }
            auto tex = RefCast<OpenglRHITexture>(in->GetOutput());
            if (!tex)
            {
                MarkProcessed();
                return;
            }
            textures.push_back(tex);
        }
        auto outTex = RefCast<OpenglRHITexture>(cashedOutput);
        if (!outTex)
        {
            MarkProcessed();
            return;
        }

        glDisable(GL_DEPTH_TEST);
        outTex->BindAsTarget();
        shader->Use();
        for (size_t i = 0; i < textures.size(); ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]->Tex);
            shader->setUniform(("inputTexture" + std::to_string(i)).c_str(), (int)i);
        }
        glBindVertexArray(FullscreenQuadHelper::GetVAO());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        outTex->UnbindTarget();
        MarkProcessed();
    }
    bool HasOwnOutput() const override
    {
        return cashedOutput != nullptr;
    }
};

class MeshBatchResourceNode : public OpenglRHINode
{
    // Кэшируем САМ пакет, чтобы не аллоцировать каждый кадр.
    // Внутри пакета рефы обновляются присваиванием — это дёшево.
    Ref<MeshBatchResource> batch;

public:
    enum Inputs : unsigned char
    {
        IMesh,
        IDiffuse,
        ISpec,
        ICount
    };

    MeshBatchResourceNode()
    {
        SetInputNodesCount(ICount);
        batch = MakeRef<MeshBatchResource>();
        cashedOutput = batch;
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        if (inputNodes.size() < ICount)
        {
            MarkProcessed();
            return;
        }

        auto mesh = RefCast<OpenglRHIMesh>(inputNodes[IMesh]->GetOutput());
        auto diff = RefCast<OpenglRHITexture>(inputNodes[IDiffuse]->GetOutput());
        auto spec = RefCast<OpenglRHITexture>(inputNodes[ISpec]->GetOutput());

        if (!mesh || !diff || !spec)
        {
            MarkProcessed();
            return;
        }

        batch->mesh = mesh;
        batch->diffuse = diff;
        batch->specular = spec;
        MarkProcessed();
    }

    bool HasOwnOutput() const override
    {
        return true;
    }
};

class BatchTransformNode : public OpenglRHINode
{
    std::vector<WeakRef<WObject>> objects;
    Ref<InstanceDataResource> instances;

public:
    BatchTransformNode(std::vector<WeakRef<WObject>> objs) : objects(std::move(objs))
    {
        instances = MakeRef<InstanceDataResource>(objects.size());
        cashedOutput = instances;

        MMatrix4f* data = instances->Data();
        for (size_t i = 0; i < objects.size(); ++i)
        {
            if (auto obj = objects[i].lock())
                obj->BindWorldTransformStorage(&data[i]);
        }
    }

    ~BatchTransformNode() override
    {
        for (auto& w : objects)
        {
            if (auto obj = w.lock())
                obj->UnbindWorldTransformStorage();
        }
    }

    void Prepair(OpenglContext&) override
    {
        WL_START_TYMETRACE;

        if (instances->gpu_data)
        {
            // Делаем клиентские записи видимыми для GPU до последующих draw call.
            glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
        }
        else
        {
            // Fallback: обычная загрузка CPU-копии.
            instances->Upload();
        }
    }

    void Execute(const OpenglContext&) override { MarkProcessed(); }
    bool HasOwnOutput() const override { return true; }
};

class OpenglInstancedMeshRenderNode : public OpenglRHINode
{
    Ref<OpenglShader> shader;
    GLint up_model;
    GLint up_view;
    GLint up_projection;
    GLint up_viewPos;
    GLint up_texture_diffuse;
    GLint up_texture_specular;

public:
    enum Inputs : unsigned char
    {
        IBatch, // MeshBatchResource
        IView, // TrivialRHIResource<MMatrix4f>
        IProj, // TrivialRHIResource<MMatrix4f>
        IViewPos, // TrivialRHIResource<MVector3f>
        IInstances, // InstanceDataResource
        ICount
    };

    OpenglInstancedMeshRenderNode(Ref<IRHIShader> sh) : shader(RefCast<OpenglShader>(sh))
    {
        SetInputNodesCount(ICount);
        if (!shader)
            return;
        up_view = shader->GetLoc("view");
        up_projection = shader->GetLoc("projection");
        up_viewPos = shader->GetLoc("viewPos");
        up_texture_diffuse = shader->GetLoc("texture_diffuse");
        up_texture_specular = shader->GetLoc("texture_specular");
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        if (inputNodes.size() < ICount)
        {
            MarkProcessed();
            return;
        }

        auto batch = RefCast<MeshBatchResource>(inputNodes[IBatch]->GetOutput());
        if (!batch || !batch->mesh || !batch->diffuse || !batch->specular)
        {
            MarkProcessed();
            return;
        }

        auto instances = RefCast<InstanceDataResource>(inputNodes[IInstances]->GetOutput());
        if (!instances || instances->count == 0)
        {
            MarkProcessed();
            return;
        }

        auto viewRes = RefCast<TrivialRHIResource<MMatrix4f>>(inputNodes[IView]->GetOutput());
        auto projRes = RefCast<TrivialRHIResource<MMatrix4f>>(inputNodes[IProj]->GetOutput());
        auto viewPosRes = RefCast<TrivialRHIResource<MVector3f>>(inputNodes[IViewPos]->GetOutput());
        auto outTex = RefCast<OpenglRHITexture>(cashedOutput);
        if (!viewRes || !projRes || !viewPosRes || !outTex)
        {
            MarkProcessed();
            return;
        }

        glEnable(GL_DEPTH_TEST);
        outTex->BindAsTarget();
        shader->Use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, batch->diffuse->Tex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, batch->specular->Tex);

        instances->buffer->BindBase(0); // SSBO → binding point 0

        shader->setUniform(up_view, viewRes->Get());
        shader->setUniform(up_projection, projRes->Get());
        shader->setUniform(up_viewPos, viewPosRes->Get());
        shader->setUniform(up_texture_diffuse, 0);
        shader->setUniform(up_texture_specular, 1);

        batch->mesh->DrawInstanced((GLsizei)instances->count);

        outTex->UnbindTarget();
        MarkProcessed();
    }
};

class OpenglBatchMeshNode : public OpenglRHINode
{
    Ref<OpenglShader> shader;
    GLint up_model;
    GLint up_view;
    GLint up_projection;
    GLint up_viewPos;
    GLint up_texture_diffuse;
    GLint up_texture_specular;

    Ref<OpenglRHIMesh> mesh;
    Ref<OpenglRHITexture> outTex;
    Ref<TrivialRHIResource<MMatrix4f>> modelRes;
    Ref<TrivialRHIResource<MMatrix4f>> viewRes;
    Ref<TrivialRHIResource<MMatrix4f>> projRes;
    Ref<TrivialRHIResource<MVector3f>> viewPosRes;
    Ref<OpenglRHITexture> diffRes;
    Ref<OpenglRHITexture> specRes;

public:
    enum Inputs : unsigned char
    {
        IMesh,
        IModel,
        IView,
        IProj,
        IViewPos,
        IDiffuse,
        ISpec,
        ICount
    };
    OpenglBatchMeshNode(Ref<IRHIShader> sh) : shader(RefCast<OpenglShader>(sh))
    {
        SetInputNodesCount(ICount);
        if (!shader)
            return;
        up_model = shader->GetLoc("model");
        up_view = shader->GetLoc("view");
        up_projection = shader->GetLoc("projection");
        up_viewPos = shader->GetLoc("viewPos");
        up_texture_diffuse = shader->GetLoc("texture_diffuse");
        up_texture_specular = shader->GetLoc("texture_specular");
    }

    void Execute(const OpenglContext& context) override
    {
        WL_START_TYMETRACE;
        if (!shader)
        {
            MarkProcessed();
            return;
        }
        if (context.rebind || !outTex || !modelRes || !viewRes || !projRes || !viewPosRes || !diffRes || !specRes)
        {
            bool correct = false;
            do
            {
                if (inputNodes.size() < ICount)
                    break;
                if (outTex = RefCast<OpenglRHITexture>(cashedOutput), !outTex)
                    break;
                if (mesh = RefCast<OpenglRHIMesh>(inputNodes[IMesh]->GetOutput()), !mesh)
                    break;
                if (modelRes = RefCast<TrivialRHIResource<MMatrix4f>>(inputNodes[IModel]->GetOutput()), !modelRes)
                    break;
                if (viewRes = RefCast<TrivialRHIResource<MMatrix4f>>(inputNodes[IView]->GetOutput()), !viewRes)
                    break;
                if (projRes = RefCast<TrivialRHIResource<MMatrix4f>>(inputNodes[IProj]->GetOutput()), !projRes)
                    break;
                if (viewPosRes = RefCast<TrivialRHIResource<MVector3f>>(inputNodes[IViewPos]->GetOutput()), !viewPosRes)
                    break;
                if (diffRes = RefCast<OpenglRHITexture>(inputNodes[IDiffuse]->GetOutput()), !diffRes)
                    break;
                if (specRes = RefCast<OpenglRHITexture>(inputNodes[ISpec]->GetOutput()), !specRes)
                    break;
                correct = true;
            } while (false);
            if (!correct)
            {
                MarkProcessed();
                return;
            }
        }
        MMatrix4f model = modelRes->Get();
        MMatrix4f view = viewRes->Get();
        MMatrix4f projection = projRes->Get();
        MVector3f viewPos = viewPosRes->Get();
        glEnable(GL_DEPTH_TEST);

        outTex->BindAsTarget();
        shader->Use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffRes->Tex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specRes->Tex);
        shader->setUniform(up_model, model);
        shader->setUniform(up_view, view);
        shader->setUniform(up_projection, projection);
        shader->setUniform(up_viewPos, viewPos);
        shader->setUniform(up_texture_diffuse, 0);
        shader->setUniform(up_texture_specular, 1);

        mesh->Draw();
        outTex->UnbindTarget();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        MarkProcessed();
    }
};

class OpenglMultipassNode : public OpenglRHINode
{
    Ref<OpenglRHITexture> outputTexture;
    int outputW = 0, outputH = 0;
public:
    explicit OpenglMultipassNode(size_t inputCount)
    {
        outputTexture = MakeRef<OpenglRHITexture>();
        SetInputNodesCount(inputCount);
    }

    bool HasOwnOutput() const override {
        return false;
    }

    void Prepair(OpenglContext& context) override 
    {}

    void Execute(const OpenglContext&) override
    {
        MarkProcessed();
    }
};

class OpenglWindowOutputNode : public OpenglRHINode
{
    Ref<OpenglRHIWindowTarget> window;
    Ref<OpenglShader> blitShader;
    Ref<OpenglRHITexture> outputTexture;
    int old_w, old_h;

public:
    OpenglWindowOutputNode(Ref<IRHIWindowTarget> winTarget) : window(RefCast<OpenglRHIWindowTarget>(winTarget))
    {
        int w, h;
        window->GetSize(w, h);
        outputTexture = MakeRef<OpenglRHITexture>();
        outputTexture->InitAsRenderTarget(w, h, PixelFormat::RGBA_8);
        cashedOutput = outputTexture;
        SetInputNodesCount(1);
        blitShader = ShaderLib::GetShader("FullscreenQuad");
    }

    bool HasOwnOutput() const override
    {
        return true;
    }
    void Prepair(OpenglContext& context) override
    {
        WL_START_TYMETRACE;
        window->GetSize(context.width, context.height);
        outputTexture->BindAsTarget();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        outputTexture->UnbindTarget();
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        if (!blitShader || !window || inputNodes.empty() || !inputNodes[0])
        {
            MarkProcessed();
            return;
        }
        auto tex = RefCast<OpenglRHITexture>(inputNodes[0]->GetOutput());
        if (!tex)
        {
            MarkProcessed();
            return;
        }
        glDisable(GL_DEPTH_TEST);
        window->Activate();
        int w, h;
        window->GetSize(w, h);
        if(old_w != w || old_h != h)
        {
            outputTexture->InitAsRenderTarget(w, h, PixelFormat::RGBA_8);
            old_w = w;
            old_h = h;
        }
        glViewport(0, 0, w, h);
        blitShader->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->Tex);
        blitShader->setUniform("screenTexture", 0);
        glBindVertexArray(FullscreenQuadHelper::GetVAO());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        MarkProcessed();
    }
};

class OpenglTextureOutputNode : public OpenglRHINode
{
    Ref<OpenglShader> copyShader;
    Ref<OpenglRHITexture> outputTexture;

public:
    OpenglTextureOutputNode(Ref<IRHITexture> outTex) : outputTexture(RefCast<OpenglRHITexture>(outTex))
    {
        cashedOutput = outputTexture;
        SetInputNodesCount(1);
        copyShader = ShaderLib::GetShader("FullscreenQuad");
    }

    bool HasOwnOutput() const override
    {
        return true;
    }
    void Prepair(OpenglContext& context) override
    {
        WL_START_TYMETRACE;
        context.width = outputTexture->width;
        context.height = outputTexture->height;
        outputTexture->BindAsTarget();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        outputTexture->UnbindTarget();
    }

    void Execute(const OpenglContext&) override
    {
        WL_START_TYMETRACE;
        if (!copyShader || !outputTexture || inputNodes.empty() || !inputNodes[0])
        {
            MarkProcessed();
            return;
        }
        auto tex = RefCast<OpenglRHITexture>(inputNodes[0]->GetOutput());
        if (!tex)
        {
            MarkProcessed();
            return;
        }
        if (tex == outputTexture)
        {
            MarkProcessed();
            return;
        }
        glDisable(GL_DEPTH_TEST);
        outputTexture->BindAsTarget();
        copyShader->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->Tex);
        copyShader->setUniform("screenTexture", 0);
        glBindVertexArray(FullscreenQuadHelper::GetVAO());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        outputTexture->UnbindTarget();
        MarkProcessed();
    }
};

#include "../../UI/UI.h"
#include "../../Text/DisplayText.h"
#include "../../Text/Font.h"

// =====================================================================
// Пер-инстансный глиф
// =====================================================================
struct TextGlyphInstance
{
    float x, y, w, h;      // px, origin — левый верх таргета
    float u0, v0, u1, v1;  // UV в атласе
    float r, g, b, a;      // цвет глифа
};


// =====================================================================
// Единый батчер текста и кеш текстур.
// Один глобальный объект — используется всеми нодами, которые рендерят
// текст и картинки виджетов.
// =====================================================================
class OpenglTextBatchDrawer
{
public:
    static OpenglTextBatchDrawer& Get()
    {
        static OpenglTextBatchDrawer inst;
        return inst;
    }

    // ------------------------------------------------------------------
    // Кеши текстур. Возвращают GLuint (0 при ошибке).
    // Первый вызов создаёт текстуру, последующие — обновляют при dirty.
    // ------------------------------------------------------------------
    GLuint EnsureImage(const Ref<Image>& img);
    GLuint EnsureAtlas(const Ref<FontAtlas>& atlas);

    // Удаляет мёртвые weak-ref'ы. Идемпотентно, вызывается из Prepair.
    void BeginFrame();

    // ------------------------------------------------------------------
    // Батч глифов.
    // ------------------------------------------------------------------
    // Добавить один глиф с указанием текстуры. Соседние глифы одной
    // текстуры автоматически сливаются в один DrawRange.
    void PushGlyph(GLuint tex, const TextGlyphInstance& g);

    // Отрисовать всё накопленное в текущий (уже забиндженный) таргет.
    // Шейдер TextInstanced + один draw на диапазон.
    void FlushText(int targetW, int targetH);

    // Сбросить пул (без отрисовки).
    void Reset();

private:
    OpenglTextBatchDrawer() { _Init(); }

    // ---- VBO/VAO ----
    GLuint _vao         = 0;
    GLuint _quadVbo     = 0;
    GLuint _instanceVbo = 0;
    size_t _capacity    = 0;

    struct DrawRange { GLuint tex; size_t start; size_t count; };
    std::vector<TextGlyphInstance> _pool;
    std::vector<DrawRange>         _ranges;

    // ---- Кеши ----
    std::unordered_map<Image*, Ref<OpenglRHITexture>>           _imageTex;
    std::unordered_map<Image*, WeakRef<Image>>                  _imageRefs;

    std::unordered_map<const FontAtlas*, Ref<OpenglRHITexture>> _atlasTex;
    std::unordered_map<const FontAtlas*, WeakRef<FontAtlas>>    _atlasRefs;

    void _Init();
    void _EnsureCapacity(size_t n);
};


// =====================================================================
// Реализация
// =====================================================================
inline GLuint OpenglTextBatchDrawer::EnsureImage(const Ref<Image>& img)
{
    if (!img) return 0;
    Image* key = img.get();

    auto it = _imageTex.find(key);
    if (it == _imageTex.end())
    {
        auto tex = MakeRef<OpenglRHITexture>();
        uint32_t iw = img->width;
        uint32_t ih = img->height;
        PixelFormat fmt = img->format;
        const uint8_t* dp = img->data;
        if (iw > 0 && ih > 0 && dp)
            tex->InitAsRenderTarget((int)iw, (int)ih, fmt, dp);
        else
        {
            static const uint8_t white[4] = { 255,255,255,255 };
            tex->InitAsRenderTarget(1, 1, PixelFormat::RGBA_8, white);
        }
        _imageTex[key] = tex;
        _imageRefs[key] = img;
        img->Update();
        return tex->Tex;
    }

    if (img->dirty)
    {
        auto& tex = it->second;
        uint32_t iw = img->width, ih = img->height;
        PixelFormat fmt = img->format;
        const uint8_t* dp = img->data;
        if (iw > 0 && ih > 0 && dp)
            tex->Update((int)iw, (int)ih, fmt, dp);
        img->Update();
    }
    return it->second->Tex;
}

inline GLuint OpenglTextBatchDrawer::EnsureAtlas(const Ref<FontAtlas>& atlas)
{
    if (!atlas || !atlas->Texture) return 0;
    const FontAtlas* key = atlas.get();
    auto& img = atlas->Texture;

    auto it = _atlasTex.find(key);
    if (it == _atlasTex.end())
    {
        auto tex = MakeRef<OpenglRHITexture>();
        tex->InitAsRenderTarget((int)img->width, (int)img->height,
                                PixelFormat::RED_8, img->data);
        _atlasTex[key] = tex;
        _atlasRefs[key] = atlas;
        img->Update();
        return tex->Tex;
    }

    if (img->dirty)
    {
        auto& tex = it->second;
        tex->Update((int)img->width, (int)img->height,
                    PixelFormat::RED_8, img->data);
        img->Update();
    }
    return it->second->Tex;
}

inline void OpenglTextBatchDrawer::BeginFrame()
{
    for (auto it = _imageTex.begin(); it != _imageTex.end(); )
    {
        auto r = _imageRefs.find(it->first);
        if (r == _imageRefs.end() || !r->second)
        {
            if (r != _imageRefs.end()) _imageRefs.erase(r);
            it = _imageTex.erase(it);
        }
        else ++it;
    }
    for (auto it = _atlasTex.begin(); it != _atlasTex.end(); )
    {
        auto r = _atlasRefs.find(it->first);
        if (r == _atlasRefs.end() || !r->second)
        {
            if (r != _atlasRefs.end()) _atlasRefs.erase(r);
            it = _atlasTex.erase(it);
        }
        else ++it;
    }
}

inline void OpenglTextBatchDrawer::PushGlyph(GLuint tex, const TextGlyphInstance& g)
{
    if (_ranges.empty() || _ranges.back().tex != tex)
        _ranges.push_back({ tex, _pool.size(), 0 });
    _ranges.back().count++;
    _pool.push_back(g);
}

inline void OpenglTextBatchDrawer::FlushText(int targetW, int targetH)
{
    if (_ranges.empty()) return;

    _EnsureCapacity(_pool.size());
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    _pool.size() * sizeof(TextGlyphInstance),
                    _pool.data());

    auto shader = ShaderLib::GetShader("TextInstanced");
    shader->Use();
    shader->setUniform("u_atlas", 0);
    shader->setUniform("u_targetSize",
                       MVector2f{ (float)targetW, (float)targetH });

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(_vao);

    GLuint bound = 0;
    for (auto& r : _ranges)
    {
        if (r.tex != bound) { glBindTexture(GL_TEXTURE_2D, r.tex); bound = r.tex; }
        glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4,
                                          (GLsizei)r.count, (GLuint)r.start);
    }
    glBindVertexArray(0);

    Reset();
}

inline void OpenglTextBatchDrawer::Reset()
{
    _pool.clear();
    _ranges.clear();
}

inline void OpenglTextBatchDrawer::_EnsureCapacity(size_t n)
{
    if (n <= _capacity) return;
    size_t newCap = std::max(_capacity * 2, n);
    newCap = std::max<size_t>(newCap, 256);

    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 newCap * sizeof(TextGlyphInstance),
                 nullptr, GL_DYNAMIC_DRAW);
    _capacity = newCap;
}

inline void OpenglTextBatchDrawer::_Init()
{
    static const float quad[8] = { 0,0,  1,0,  0,1,  1,1 };

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, _quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glGenBuffers(1, &_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(TextGlyphInstance),
                 nullptr, GL_DYNAMIC_DRAW);
    _capacity = 256;

    const GLsizei S = sizeof(TextGlyphInstance);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, S,
                          (void*)offsetof(TextGlyphInstance, x));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, S,
                          (void*)offsetof(TextGlyphInstance, w));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, S,
                          (void*)offsetof(TextGlyphInstance, u0));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, S,
                          (void*)offsetof(TextGlyphInstance, r));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

class OpenglWidgetRenderNode : public OpenglRHINode
{
    WeakRef<Widget> widget;
    Ref<OpenglRHITexture> outputTexture;
    int outputW = 0, outputH = 0;

    struct DrawOp
    {
        enum Type : uint8_t { Image, Text } type;
        GLuint tex = 0;
        int ix = 0, iy = 0, iw = 0, ih = 0;
        size_t glyphStart = 0;
        size_t glyphCount = 0;
    };
    std::vector<DrawOp> ops;
    std::vector<TextGlyphInstance> glyphPool;

    size_t lastHash = 0;

public:
    explicit OpenglWidgetRenderNode(Ref<Widget> w) : widget(std::move(w)) {}

    bool HasOwnOutput() const override { return false; }

    void Prepair(OpenglContext& context) override
    {
        auto& drawer = OpenglTextBatchDrawer::Get();
        drawer.BeginFrame();

        const int W = context.width;
        const int H = context.height;
        if (W <= 0 || H <= 0) return;

        if (outputW != W || outputH != H)
        {
            outputW = W;
            outputH = H;
            lastHash = 0;
        }

        auto w = widget.lock();
        if (!w) return;

        auto listItems = w->Render((double)W, (double)H);
        std::vector<Widget::ToRender> items(listItems.begin(), listItems.end());

        if (items.empty())
        {
            if (!ops.empty()) { ops.clear(); glyphPool.clear(); lastHash = 0; }
            return;
        }

        // ---------- Хэш + детект dirty + обновление текстур ----------
        size_t hash = 0;
        auto mix = [&hash](size_t v) {
            hash ^= v + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
        };
        bool anyDirty = false;

        for (auto& it : items)
        {
            mix(reinterpret_cast<size_t>(it.data.get()));
            mix(reinterpret_cast<size_t>(it.text.get()));
            mix((size_t)(uint32_t)it.Z);
            uint64_t x, y, ww, hh;
            std::memcpy(&x,  &it.X, 8);
            std::memcpy(&y,  &it.Y, 8);
            std::memcpy(&ww, &it.W, 8);
            std::memcpy(&hh, &it.H, 8);
            mix((size_t)x); mix((size_t)y); mix((size_t)ww); mix((size_t)hh);

            if (it.data)
            {
                if (it.data->dirty) anyDirty = true;
                drawer.EnsureImage(it.data);
            }

            if (it.text)
            {
                for (auto& p : it.text->GetLayout())
                {
                    if (!p.Sym || !p.Sym->Atlas) continue;
                    if (p.Sym->Atlas->Texture && p.Sym->Atlas->Texture->dirty)
                        anyDirty = true;
                    drawer.EnsureAtlas(p.Sym->Atlas);
                }
            }
        }

        if (hash == lastHash && !anyDirty)
            return;
        lastHash = hash;

        // ---------- Сортировка по Z ----------
        std::sort(items.begin(), items.end(),
                  [](const Widget::ToRender& a, const Widget::ToRender& b)
                  { return a.Z < b.Z; });

        // ---------- Сборка draw-ops ----------
        ops.clear();
        glyphPool.clear();

        for (auto& item : items)
        {
            // --- Image ---
            if (item.data)
            {
                GLuint tex = drawer.EnsureImage(item.data);
                if (!tex) continue;

                const int x = (int)std::floor(item.X);
                const int y = (int)std::floor(item.Y);
                const int w = (int)std::ceil(item.W);
                const int h = (int)std::ceil(item.H);
                if (w <= 0 || h <= 0) continue;

                DrawOp op;
                op.type = DrawOp::Image;
                op.tex  = tex;
                op.ix = x; op.iy = y; op.iw = w; op.ih = h;
                ops.push_back(op);
                continue;
            }

            // --- Text ---
            if (item.text)
            {
                const auto& layout = item.text->GetLayout();
                for (auto& p : layout)
                {
                    if (!p.Font || !p.Sym || !p.Sym->bInAtlas) continue;
                    if (p.Sym->PixelW == 0 || p.Sym->PixelH == 0) continue;
                    if (!p.Sym->Atlas) continue;

                    GLuint gtex = drawer.EnsureAtlas(p.Sym->Atlas);
                    if (!gtex) continue;

                    TextGlyphInstance g;
                    g.x  = (float)std::floor(item.X + p.X);
                    g.y  = (float)std::floor(item.Y + p.Y);
                    g.w  = (float)p.Sym->PixelW;
                    g.h  = (float)p.Sym->PixelH;
                    g.u0 = p.Sym->U0; g.v0 = p.Sym->V0;
                    g.u1 = p.Sym->U1; g.v1 = p.Sym->V1;
                    g.r = ((p.Color >>  0) & 0xff) / 255.0f;
                    g.g = ((p.Color >>  8) & 0xff) / 255.0f;
                    g.b = ((p.Color >> 16) & 0xff) / 255.0f;
                    g.a = ((p.Color >> 24) & 0xff) / 255.0f;

                    if (!ops.empty()
                        && ops.back().type == DrawOp::Text
                        && ops.back().tex  == gtex)
                    {
                        ops.back().glyphCount++;
                    }
                    else
                    {
                        DrawOp op;
                        op.type       = DrawOp::Text;
                        op.tex        = gtex;
                        op.glyphStart = glyphPool.size();
                        op.glyphCount = 1;
                        ops.push_back(op);
                    }
                    glyphPool.push_back(g);
                }
            }
        }
    }

    void Execute(const OpenglContext&) override
    {
        outputTexture = RefCast<OpenglRHITexture>(cashedOutput);
        if (!outputTexture || ops.empty())
        {
            MarkProcessed();
            return;
        }

        outputTexture->BindAsTarget();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto& drawer = OpenglTextBatchDrawer::Get();
        drawer.Reset();

        const MVector2f targetSize{ (float)outputW, (float)outputH };
        auto shaderImg = ShaderLib::GetShader("WidgetBlit");

        for (auto& op : ops)
        {
            if (op.type == DrawOp::Image)
            {
                // Сначала — накопленный текст (порядок по Z сохраняется).
                drawer.FlushText(outputW, outputH);

                shaderImg->Use();
                shaderImg->setUniform("screenTexture", 0);
                shaderImg->setUniform("u_targetSize", targetSize);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, op.tex);
                shaderImg->setUniform("u_rect",
                    MVector4f{ (float)op.ix, (float)op.iy,
                               (float)op.iw, (float)op.ih });
                glBindVertexArray(FullscreenQuadHelper::GetVAO());
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            else
            {
                for (size_t i = 0; i < op.glyphCount; ++i)
                    drawer.PushGlyph(op.tex, glyphPool[op.glyphStart + i]);
            }
        }
        drawer.FlushText(outputW, outputH);

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        outputTexture->UnbindTarget();

        MarkProcessed();
    }
};

class OpenglTextRenderNode : public OpenglRHINode
{
    WeakRef<DisplayText> text;
    Ref<OpenglRHITexture> outputTexture;
    int outputW = 0, outputH = 0;

    // Отсортированные по атласу диапазоны в pool.
    std::vector<TextGlyphInstance> glyphPool;
    std::vector<std::pair<GLuint, std::pair<size_t, size_t>>> draws;

    size_t lastHash = 0;

public:
    explicit OpenglTextRenderNode(Ref<DisplayText> t) : text(std::move(t)) {}

    bool HasOwnOutput() const override { return false; }

    void Prepair(OpenglContext& ctx) override
    {
        auto& drawer = OpenglTextBatchDrawer::Get();
        drawer.BeginFrame();

        const int W = ctx.width, H = ctx.height;
        if (W <= 0 || H <= 0) return;

        if (outputW != W || outputH != H)
        {
            outputW = W;
            outputH = H;
            lastHash = 0;
        }

        auto t = text.lock();
        if (!t) { glyphPool.clear(); draws.clear(); lastHash = 0; return; }

        const auto& layout = t->GetLayout();

        // ---------- Хэш + dirty + обновление атласов ----------
        size_t h = 0;
        auto mix = [&h](size_t v) {
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };
        mix(layout.size());

        bool anyDirty = false;
        for (auto& p : layout)
        {
            mix(reinterpret_cast<size_t>(p.Sym.get()));
            mix(reinterpret_cast<size_t>(p.Font.get()));
            mix((size_t)p.Color);

            if (p.Sym && p.Sym->Atlas)
            {
                if (p.Sym->Atlas->Texture && p.Sym->Atlas->Texture->dirty)
                    anyDirty = true;
                drawer.EnsureAtlas(p.Sym->Atlas);
            }
        }

        if (h == lastHash && !anyDirty)
            return;
        lastHash = h;

        // ---------- Сборка ----------
        glyphPool.clear();
        draws.clear();

        for (auto& p : layout)
        {
            if (!p.Font || !p.Sym || !p.Sym->bInAtlas) continue;
            if (p.Sym->PixelW == 0 || p.Sym->PixelH == 0) continue;
            if (!p.Sym->Atlas) continue;

            GLuint gtex = drawer.EnsureAtlas(p.Sym->Atlas);
            if (!gtex) continue;

            TextGlyphInstance g;
            g.x  = (float)std::floor(p.X);
            g.y  = (float)std::floor(p.Y);
            g.w  = (float)p.Sym->PixelW;
            g.h  = (float)p.Sym->PixelH;
            g.u0 = p.Sym->U0; g.v0 = p.Sym->V0;
            g.u1 = p.Sym->U1; g.v1 = p.Sym->V1;
            g.r = ((p.Color >>  0) & 0xff) / 255.0f;
            g.g = ((p.Color >>  8) & 0xff) / 255.0f;
            g.b = ((p.Color >> 16) & 0xff) / 255.0f;
            g.a = ((p.Color >> 24) & 0xff) / 255.0f;

            if (!draws.empty() && draws.back().first == gtex)
                draws.back().second.second++;
            else
                draws.push_back({ gtex, { glyphPool.size(), 1 } });
            glyphPool.push_back(g);
        }
    }

    void Execute(const OpenglContext&) override
    {
        outputTexture = RefCast<OpenglRHITexture>(cashedOutput);
        if (!outputTexture || draws.empty())
        {
            MarkProcessed();
            return;
        }

        outputTexture->BindAsTarget();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto& drawer = OpenglTextBatchDrawer::Get();
        drawer.Reset();

        for (auto& d : draws)
            for (size_t i = 0; i < d.second.second; ++i)
                drawer.PushGlyph(d.first, glyphPool[d.second.first + i]);

        drawer.FlushText(outputW, outputH);

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        outputTexture->UnbindTarget();

        MarkProcessed();
    }
};

#endif
