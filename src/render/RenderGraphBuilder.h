#ifndef __RENDERGRAPHBUILDER_H__
#define __RENDERGRAPHBUILDER_H__

#include "../core/core.h"
#include "IRHIHelper.h"
#include <unordered_map>
#include <string>

class UI;
class World;
class IWindow;
class Texture;
class Camera;
class RGBNode;
class Image;

class RenderGraphBuilder
{
public:
    static Ref<RenderGraphBuilder> MakeFromTarget(Ref<IWindow> window);
    static Ref<RenderGraphBuilder> MakeFromTarget(Ref<Texture> texture);

    Ref<RGBNode> Root()
    {
        return _rootNode;
    }

    Ref<IRHIRenderGraph> Build()
    {
        WL_START_TYMETRACE;
        if (!_rhi)
            _rhi = IRHIHelper::Get()->GenRenderGraphBuilder();
        if (_rhi)
        {
            bool recompile = _dirty.exchange(false);
            return _rhi->Build(this, recompile);
        }
        return nullptr;
    }

    void SetDirty() const
    {
        _dirty.store(true);
    }

private:
    mutable std::atomic<bool> _dirty{true};
    Ref<RGBNode> _rootNode;
    Ref<IRHIRenderGraphBuilder> _rhi;
};


#define INIT_RGB_NODE(Class)          \
    virtual size_t GetType() override \
    {                                 \
        return GID(Class);            \
    }

class RGBNode
{
public:
    enum PinType : unsigned int
    {
        PinFloat = 0x00,
        PinFloat2,
        PinFloat3,
        PinFloat4,
        PinFloat22,
        PinFloat33,
        PinFloat44,
        PinColor = PinFloat4,
        PinTextureR = 0x10,
        PinTextureRG,
        PinTextureRGB,
        PinTextureRGBA
    };

    struct Pin
    {
        std::string name;
        PinType type;
        Ref<RGBNode> target = nullptr;
        size_t target_pin_id = 0;
    };

    void SetName(const std::string &name)
    {
        _name = name;
    }
    std::string GetName() const
    {
        return _name;
    }

    void SetRGB(RenderGraphBuilder *rgb)
    {
        _RGB = rgb;
    }
    RenderGraphBuilder *GetRGB() const
    {
        return _RGB;
    }

    std::vector<Pin> GetInputs() const
    {
        return _inputs;
    }
    std::vector<Pin> GetOutputs() const
    {
        return _outputs;
    }

    Pin *GetInputByName(const std::string &name)
    {
        if (_RGB)
            _RGB->SetDirty();
        for (auto &pin : _inputs)
            if (pin.name == name)
                return &pin;
        return nullptr;
    }
    Pin *GetInputById(const size_t &id)
    {
        if (_RGB)
            _RGB->SetDirty();
        if (id < _inputs.size())
            return &_inputs[id];
        return nullptr;
    }
    size_t GetInputIdByName(const std::string &name)
    {
        if (_RGB)
            _RGB->SetDirty();
        for (size_t i = 0; i < _inputs.size(); ++i)
            if (_inputs[i].name == name)
                return i;
        return 0xffffffffffffffff;
    }

    Pin *GetOutputByName(const std::string &name)
    {
        for (auto &pin : _outputs)
            if (pin.name == name)
                return &pin;
        return nullptr;
    }
    Pin *GetOutputById(const size_t &id)
    {
        if (_RGB)
            _RGB->SetDirty();
        if (id < _outputs.size())
            return &_outputs[id];
        return nullptr;
    }
    size_t GetOutputIdByName(const std::string &name)
    {
        if (_RGB)
            _RGB->SetDirty();
        for (size_t i = 0; i < _outputs.size(); ++i)
            if (_outputs[i].name == name)
                return i;
        return 0xffffffffffffffff;
    }

    void BindInput(const size_t &my_input_id, Ref<RGBNode> other, const size_t &other_output_id)
    {
        if (_RGB)
            _RGB->SetDirty();
        if (my_input_id < _inputs.size())
        {
            _inputs[my_input_id].target = other;
            _inputs[my_input_id].target_pin_id = other_output_id;
        }
    }
    void BindInput(const std::string &my_input_name, Ref<RGBNode> other, const std::string &other_output_name)
    {
        if (_RGB)
            _RGB->SetDirty();
        auto my_id = GetInputIdByName(my_input_name);
        auto other_id = other->GetOutputIdByName(other_output_name);
        if (my_id != 0xffffffffffffffff && other_id != 0xffffffffffffffff)
            BindInput(my_id, other, other_id);
    }

    virtual size_t GetType() = 0;

    bool IsDirty() const
    {
        return _dirty;
    }

    void SetDirty() const
    {
        _dirty = true;
    }

    void ResetDirty() const
    {
        _dirty = false;
    }

    virtual void Update() {}

protected:
    mutable bool _dirty = true;
    std::vector<Pin> _inputs;
    std::vector<Pin> _outputs;
    std::unordered_map<std::string, Data> _props;
    RenderGraphBuilder *_RGB = nullptr;
    std::string _name;
};

class RGBWindowOutputNode : public RGBNode
{
    friend class RenderGraphBuilder;

public:
    INIT_RGB_NODE(RGBWindowOutputNode)
    RGBWindowOutputNode()
    {
        _inputs.resize(1);
        _inputs[0].name = "Color";
        _inputs[0].type = PinTextureRGBA;
    }
    Ref<IRHIWindowTarget> windowTarget;
};

class RGBTextureOutputNode : public RGBNode
{
    friend class RenderGraphBuilder;

public:
    INIT_RGB_NODE(RGBTextureOutputNode)
    RGBTextureOutputNode()
    {
        _inputs.resize(1);
        _inputs[0].name = "Color";
        _inputs[0].type = PinTextureRGBA;
    }
    Ref<IRHITexture> textureTarget;
};

inline Ref<RenderGraphBuilder> RenderGraphBuilder::MakeFromTarget(Ref<IWindow> window)
{
    auto builder = MakeRef<RenderGraphBuilder>();
    auto outputNode = MakeRef<RGBWindowOutputNode>();
    outputNode->windowTarget = IRHIHelper::Get()->GenWindowTarget();
    outputNode->windowTarget->Update(window);
    builder->_rootNode = outputNode;
    return builder;
}

inline Ref<RenderGraphBuilder> RenderGraphBuilder::MakeFromTarget(Ref<Texture> texture)
{
    auto builder = MakeRef<RenderGraphBuilder>();
    auto outputNode = MakeRef<RGBTextureOutputNode>();
    outputNode->textureTarget = IRHIHelper::Get()->GenTexture();
    outputNode->textureTarget->Update(texture);
    builder->_rootNode = outputNode;
    return builder;
}

class RGBWorldRender : public RGBNode
{
public:
    INIT_RGB_NODE(RGBWorldRender)

    static Ref<RGBWorldRender> Make(Ref<World> world, Ref<Camera> camera, int width, int height)
    {
        auto ret = MakeRef<RGBWorldRender>();
        ret->_world = world;
        ret->_camera = camera;
        ret->_width = width;
        ret->_height = height;
        ret->_outputs.resize(1);
        ret->_outputs[0].name = "Color";
        ret->_outputs[0].type = PinTextureRGBA;
        return ret;
    }

    Ref<World> _world;
    Ref<Camera> _camera;
    int _width;
    int _height;

    size_t oldhash = 0;
    
    virtual void Update() 
    {
        const auto& objs = _world->GetObjects();
        size_t hash = 0;
        for (auto obj : objs)
        {
            hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)obj.get());
            for (auto child : obj->GetAllChildren()) 
            {
                hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)child.get());
            }
            for (auto comp : obj->GetAllComponents()) 
            {
                hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)comp.get());
            }
        }
        if(oldhash != hash)
            SetDirty();
        oldhash = hash;
    }
};

class RGBImageNode : public RGBNode {
public:
    INIT_RGB_NODE(RGBImageNode)

    static Ref<RGBImageNode> Make(Ref<Image> image) {
        auto ret = MakeRef<RGBImageNode>();
        ret->_image = image;
        // один выход — текстура
        ret->_outputs.resize(1);
        ret->_outputs[0].name = "Color";
        ret->_outputs[0].type = PinTextureRGBA;
        return ret;
    }

    Ref<Image> _image;
};

#endif