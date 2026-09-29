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

#endif