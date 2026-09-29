#ifndef __OPENGLRENDERGRAPHBUILDER_H__
#define __OPENGLRENDERGRAPHBUILDER_H__

#include "../../core/core.h"
#include "../IRHIHelper.h"
#include "../RenderGraphBuilder.h"
#include "OpenglRHIResources.h"
#include "OpenglRenderNodes.h"
#include "OpenglShaderLib.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>

class OpenglRenderGraph : public IRHIRenderGraph
{
public:
    std::vector<Ref<OpenglRHINode>> nodes;
    Ref<OpenglRHINode> rootNode;
    std::vector<Ref<OpenglRHINode>> cachedOrder;
    OpenglContext context;

    void Execute() override
    {
        Execute(OpenglContext());
    }

    void Execute(const OpenglContext &inContext)
    {
        WL_START_TYMETRACE;
        context = inContext;
        rootNode->Prepair(context);
        if (cachedOrder.empty())
        {
            context.rebind = true;
            BuildExecutionOrder();
        }

        for (auto &n : cachedOrder)
        {
            n->ResetProcessed();
            n->Prepair(context);
        }

        bool cacheValid = true;
        for (auto &node : cachedOrder)
        {
            auto nodePtr = node.get();
            if (nodePtr && nodePtr->CanExecute())
            {
                nodePtr->ResolveOutput();
                nodePtr->Execute(context);
                if (!nodePtr->IsProcessed())
                {
                    cacheValid = false;
                    break;
                }
            }
            else
            {
                cacheValid = false;
                break;
            }
        }

        if (!cacheValid)
            cachedOrder.clear();
    }

private:
    void BuildExecutionOrder()
    {
        WL_START_TYMETRACE;
        for (auto &n : nodes)
            n->ResetProcessed();

        std::list<Ref<OpenglRHINode>> unsorted;
        std::list<Ref<OpenglRHINode>> toVisit;
        std::unordered_set<Ref<OpenglRHINode>> visited;
        std::unordered_map<Ref<OpenglRHINode>, std::list<Ref<OpenglRHINode>>> parents;
        toVisit.push_back(rootNode);

        while (!toVisit.empty())
        {
            auto current = toVisit.front();
            toVisit.pop_front();
            if (!current || visited.count(current))
                continue;
            visited.insert(current);
            unsorted.push_front(current);

            for (auto &in : current->GetInputs())
            {
                auto res = parents.try_emplace(in, std::list<Ref<OpenglRHINode>>());
                res.first->second.push_back(current);
                toVisit.push_back(in);
            }

            for (auto &req : current->requirementNodes)
                toVisit.push_back(req);
        }

        for (auto node : visited)
        {
            auto res = parents.try_emplace(node, std::list<Ref<OpenglRHINode>>());
            node->SetParent(res.first->second.size() == 1 ? res.first->second.front() : nullptr);
            node->Prepair(context);
        }

        std::vector<Ref<OpenglRHINode>> sortedList;
        while (!unsorted.empty())
        {
            bool anyProcessed = false;
            for (auto it = unsorted.begin(); it != unsorted.end();)
            {
                auto node = *it;
                if (node->CanExecute())
                {
                    if (!node->IsProcessed())
                    {
                        node->ResolveOutput();
                        node->Execute(context);
                    }
                    if (node->IsProcessed())
                    {
                        sortedList.push_back(node);
                        it = unsorted.erase(it);
                        anyProcessed = true;
                    }
                    else
                    {
                        ++it;
                    }
                }
                else
                {
                    ++it;
                }
            }
            if (!anyProcessed)
                break;
        }
        cachedOrder = sortedList;
    }
};

// ============================================================================
// Строитель низкоуровневого графа
// ============================================================================
class OpenglRenderGraphBuilder : public IRHIRenderGraphBuilder
{
public:
    struct ConvertersReturnValue
    {
        struct MappedInputPin
        {
            int rgbInputIdx;
            int rhiNodeIdx;
            int rhiInputIdx;
        };
        struct MappedOutputPin
        {
            int rgbOutputIdx;
            int rhiNodeIdx;
        };
        std::vector<Ref<OpenglRHINode>> nodes;
        std::vector<MappedInputPin> mappedInputs;
        std::vector<MappedOutputPin> mappedOutputs;
    };

    using ConverterFunc = std::function<ConvertersReturnValue(OpenglRenderGraphBuilder &, RGBNode *)>;

    static std::unordered_map<size_t, ConverterFunc> &GetConverters()
    {
        static std::unordered_map<size_t, ConverterFunc> map;
        return map;
    }

    template<typename T, size_t ID>
    static bool RegisterConverter(ConverterFunc func)
    {
        GetConverters()[ID] = func;
        return true;
    }

    std::vector<Ref<OpenglRHINode>> nodes;
    Ref<OpenglRHINode> rootNode;
    Ref<OpenglRenderGraph> cachedGraph;
    std::vector<Ref<OpenglRHITexture>> transientTextures;

    // Кеш ресурсных нод (текстура -> OpenglTextureResourceNode)
    std::unordered_map<IRHITexture *, Ref<OpenglTextureResourceNode>> textureResourceCache;
    std::unordered_map<MVector3f, Ref<ColorTextureResourceNode>> colorTextureCache;
    std::unordered_map<IRHIMesh *, Ref<MeshRecourceNode>> meshResourceCache;

    Ref<OpenglTextureResourceNode> GetOrCreateTextureNode(Ref<IRHITexture> tex)
    {
        if (!tex)
            return NullTextureResourceNode::Get();
        auto it = textureResourceCache.find(tex.get());
        if (it != textureResourceCache.end())
            return it->second;
        auto node = MakeRef<OpenglTextureResourceNode>(tex);
        textureResourceCache[tex.get()] = node;
        nodes.push_back(node); // добавим в общий список узлов графа
        return node;
    }

    Ref<ColorTextureResourceNode> GetOrCreateColorTextureNode(MVector3f color)
    {
        auto it = colorTextureCache.find(color);
        if (it != colorTextureCache.end())
            return it->second;
        auto node = MakeRef<ColorTextureResourceNode>(color);
        colorTextureCache[color] = node;
        nodes.push_back(node);
        return node;
    }

    Ref<MeshRecourceNode> GetOrCreateMeshNode(Ref<IRHIMesh> mesh)
    {
        if (!mesh)
            return nullptr;
        auto it = meshResourceCache.find(mesh.get());
        if (it != meshResourceCache.end())
            return it->second;
        auto node = MakeRef<MeshRecourceNode>(mesh);
        meshResourceCache[mesh.get()] = node;
        nodes.push_back(node);
        return node;
    }

    Ref<OpenglRHITexture> GetTransientTexture(int w, int h, PixelFormat fmt = PixelFormat::RGBA_8)
    {
        auto tex = MakeRef<OpenglRHITexture>();
        tex->InitAsRenderTarget(w, h, fmt);
        transientTextures.push_back(tex);
        return tex;
    }

    void Compile(RenderGraphBuilder *rgb)
    {
        WL_START_TYMETRACE;
        nodes.clear();
        textureResourceCache.clear();
        transientTextures.clear();
        rootNode = nullptr;

        std::unordered_map<RGBNode *, ConvertersReturnValue> conversionResults;

        std::function<void(RGBNode *)> traverse = [&](RGBNode *rgbNode) {
            if (!rgbNode || conversionResults.find(rgbNode) != conversionResults.end())
                return;

            for (auto &pin : rgbNode->GetInputs())
                if (pin.target)
                    traverse(pin.target.get());

            auto it = GetConverters().find(rgbNode->GetType());
            if (it != GetConverters().end())
            {
                auto result = it->second(*this, rgbNode);
                conversionResults[rgbNode] = result;
                for (auto &n : result.nodes)
                    nodes.push_back(n);
                if (rgbNode == rgb->Root().get() && !result.nodes.empty())
                    rootNode = result.nodes.back();
            }
        };

        traverse(rgb->Root().get());

        // Связывание входов
        for (auto &[rgbNode, result] : conversionResults)
        {
            for (auto &mapIn : result.mappedInputs)
            {
                auto *rgbPin = rgbNode->GetInputById(mapIn.rgbInputIdx);
                if (!rgbPin || !rgbPin->target)
                    continue;
                auto srcResultIt = conversionResults.find(rgbPin->target.get());
                if (srcResultIt == conversionResults.end())
                    continue;
                auto &srcResult = srcResultIt->second;
                for (auto &mapOut : srcResult.mappedOutputs)
                {
                    if (mapOut.rgbOutputIdx == rgbPin->target_pin_id)
                    {
                        auto srcNode = srcResult.nodes[mapOut.rhiNodeIdx];
                        auto dstNode = result.nodes[mapIn.rhiNodeIdx];
                        if (srcNode && dstNode)
                            dstNode->SetInputNode(mapIn.rhiInputIdx, srcNode);
                    }
                }
            }
        }
    }

    Ref<IRHIRenderGraph> Build(RenderGraphBuilder *rgb, bool recompile) override
    {
        WL_START_TYMETRACE;
        if (!cachedGraph || recompile)
        {
            cachedGraph = MakeRef<OpenglRenderGraph>();
            Compile(rgb);
            cachedGraph->nodes = nodes;
            cachedGraph->rootNode = rootNode;
        }
        return cachedGraph;
    }
};

// Макросы регистрации конвертеров
#define REGISTER_RGB_CONVERTER_EXT(RGBType, func)                                                         \
    namespace __RGB_CONVERTERS__                                                                          \
    {                                                                                                     \
    static bool RGBType##_reg = OpenglRenderGraphBuilder::RegisterConverter<RGBType, GID(RGBType)>(func); \
    }

#define REGISTER_RGB_CONVERTER(RGBType, ...)                                                        \
    static OpenglRenderGraphBuilder::ConvertersReturnValue RGBType##_converter_func(                \
      OpenglRenderGraphBuilder &, RGBNode *);                                                       \
    namespace __RGB_CONVERTERS__                                                                    \
    {                                                                                               \
    static bool RGBType##_reg =                                                                     \
      OpenglRenderGraphBuilder::RegisterConverter<RGBType, GID(RGBType)>(RGBType##_converter_func); \
    }                                                                                               \
    static OpenglRenderGraphBuilder::ConvertersReturnValue RGBType##_converter_func(                \
      OpenglRenderGraphBuilder &builder, RGBNode *node) __VA_ARGS__

// Регистрация конвертеров (оставим отдельно в конце файла или перенесём в .cpp, но для краткости здесь)
REGISTER_RGB_CONVERTER(RGBWindowOutputNode, {
    auto *winNode = static_cast<RGBWindowOutputNode *>(node);
    if (!winNode)
        return {};
    WL_START_TYMETRACE;
    auto rhiNode = MakeRef<OpenglWindowOutputNode>(winNode->windowTarget);
    rhiNode->SetInputNodesCount(1); // гарантируем размер
    OpenglRenderGraphBuilder::ConvertersReturnValue ret;
    ret.nodes = {rhiNode};
    ret.mappedInputs.push_back({0, 0, 0});
    return ret;
});

REGISTER_RGB_CONVERTER(RGBTextureOutputNode, {
    auto *texNode = static_cast<RGBTextureOutputNode *>(node);
    if (!texNode)
        return {};
    WL_START_TYMETRACE;
    auto rhiNode = MakeRef<OpenglTextureOutputNode>(texNode->textureTarget);
    rhiNode->SetInputNodesCount(1);
    OpenglRenderGraphBuilder::ConvertersReturnValue ret;
    ret.nodes = {rhiNode};
    ret.mappedInputs.push_back({0, 0, 0});
    return ret;
});

#include "../../World/Camera.h"
#include "../../World/World.h"
#include "../../World/Light.h"
#include "../../graphic/Map.h"
#include "../../World/StaticMeshActor.h"

static OpenglRenderGraphBuilder::ConvertersReturnValue RGBWorldRender_converter_func(OpenglRenderGraphBuilder &builder,
  RGBNode *node)
{
    auto *wr = static_cast<RGBWorldRender *>(node);
    if (!wr || !wr->_world || !wr->_camera)
        return {};
    WL_START_TYMETRACE;

    auto seqNode = MakeRef<OpenglSequenceNode>();
    OpenglRenderGraphBuilder::ConvertersReturnValue ret;
    ret.nodes.push_back(seqNode);
    ret.mappedOutputs.push_back({0, 0});

    auto viewNode = MakeRef<CameraViewMatrixNode>(wr->_camera);
    ret.nodes.push_back(viewNode);
    auto projNode = MakeRef<CameraProjectionMatrixNode>(wr->_camera, (float)wr->_width / wr->_height);
    ret.nodes.push_back(projNode);
    auto viewPosNode = MakeRef<ViewPosNode>(wr->_camera);
    ret.nodes.push_back(viewPosNode);

    struct Group {
        Ref<MeshRecourceNode> mesh;
        Ref<OpenglTextureResourceNode> diff, spec;
        std::vector<Ref<WObject>> objs;
    };
    std::unordered_map<Mesh*, Group> groups;

    std::function<void(Ref<WObject>)> traverse = [&](Ref<WObject> obj) {
        if (auto meshComp = RefCast<StaticMeshComponent>(obj))
        {
            Ref<Mesh> mesh = meshComp->mesh;
            if (meshComp->isVisible && mesh)
            {
                mesh->Update();
                auto meshNode = builder.GetOrCreateMeshNode(mesh->rhimesh);

                Ref<OpenglTextureResourceNode> diffNode, specNode;
                if (auto mat = meshComp->material.get())
                {
                    auto diffMap = mat->_maps.count("diffuse") ? mat->_maps["diffuse"] : nullptr;
                    if (diffMap)
                    {
                        if (diffMap->texture)
                        {
                            diffMap->texture->Update();
                            diffNode = builder.GetOrCreateTextureNode(diffMap->texture->rhi);
                        }
                        else
                        {
                            diffNode = builder.GetOrCreateColorTextureNode(diffMap->color);
                        }
                    }
                    auto specMap = mat->_maps.count("specular") ? mat->_maps["specular"] : nullptr;
                    if (specMap)
                    {
                        if (specMap->texture)
                        {
                            specMap->texture->Update();
                            specNode = builder.GetOrCreateTextureNode(specMap->texture->rhi);
                        }
                        else
                        {
                            specNode = builder.GetOrCreateColorTextureNode(specMap->color);
                        }
                    }
                }
                if (!diffNode)
                    diffNode = NullTextureResourceNode::Get();
                if (!specNode)
                    specNode = NullTextureResourceNode::Get();

                auto& res = groups.try_emplace(mesh.get(), Group{meshNode, diffNode, specNode, {obj}});

                if(!res.second)
                {
                    res.first->second.objs.push_back(obj);
                }
            }
        }
        for (auto &child : obj->GetChildren())
            traverse(child);
        for (auto &comp : obj->GetComponents())
            traverse(comp);
    };

    for (auto &obj : wr->_world->GetObjects())
        traverse(obj);

    for (auto& [key, g] : groups)
    {
        if (!g.mesh || !g.diff || !g.spec || g.objs.size() == 0) continue;
        if(g.objs.size() == 1)
        {
            auto shader = ShaderLib::GetShader("Mesh3D");
            auto transformNode = MakeRef<WObjectTransformNode>(g.objs[0]);
            auto batchNode = MakeRef<OpenglBatchMeshNode>(shader);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IMesh, g.mesh);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IModel, transformNode);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IView, viewNode);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IProj, projNode);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IViewPos, viewPosNode);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::IDiffuse, g.diff);
            batchNode->SetInputNode(OpenglBatchMeshNode::Inputs::ISpec, g.spec);
            ret.nodes.push_back(batchNode);

            seqNode->AddSequenceNode(batchNode);

        }
        else
        {
            auto shader = ShaderLib::GetShader("InstMesh3D");
            auto batchNode = MakeRef<MeshBatchResourceNode>();
            batchNode->SetInputNode(MeshBatchResourceNode::IMesh,    g.mesh);
            batchNode->SetInputNode(MeshBatchResourceNode::IDiffuse, g.diff);
            batchNode->SetInputNode(MeshBatchResourceNode::ISpec,    g.spec);
            ret.nodes.push_back(batchNode);

            std::vector<WeakRef<WObject>> weak;
            weak.reserve(g.objs.size());
            for(auto obj : g.objs)
            {
                weak.push_back(obj);
            }
            auto transformNode = MakeRef<BatchTransformNode>(std::move(weak));
            ret.nodes.push_back(transformNode);

            auto instNode = MakeRef<OpenglInstancedMeshRenderNode>(shader);
            instNode->SetInputNode(OpenglInstancedMeshRenderNode::IBatch,     batchNode);
            instNode->SetInputNode(OpenglInstancedMeshRenderNode::IView,      viewNode);
            instNode->SetInputNode(OpenglInstancedMeshRenderNode::IProj,      projNode);
            instNode->SetInputNode(OpenglInstancedMeshRenderNode::IViewPos,   viewPosNode);
            instNode->SetInputNode(OpenglInstancedMeshRenderNode::IInstances, transformNode);
            ret.nodes.push_back(instNode);

            seqNode->AddSequenceNode(instNode);

        }
    }
    return ret;
}
REGISTER_RGB_CONVERTER_EXT(RGBWorldRender, RGBWorldRender_converter_func);

REGISTER_RGB_CONVERTER(RGBImageNode, {
    auto *imgNode = static_cast<RGBImageNode *>(node);
    if (!imgNode || !imgNode->_image)
        return {};
    WL_START_TYMETRACE;
    auto rhiNode = MakeRef<OpenglImageResourceNode>(imgNode->_image);
    OpenglRenderGraphBuilder::ConvertersReturnValue ret;
    ret.nodes = {rhiNode};
    ret.mappedOutputs.push_back({0, 0});
    return ret;
});

#endif