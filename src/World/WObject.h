#pragma once
#include "../core/core.h"
#include <unordered_map>

struct World;
class WObject;
class WComponent;
class WActor;

WENGINE_CLASS(Serealizable)
class WObject : public CoreObject
{
    friend class World;
public:
    enum class TransformMode
    {
        Lazy,
        Strong
    };
    WObject(const std::string& name = "Object");
    virtual ~WObject() = default;

    virtual bool IsComponent() const
    {
        return !owner.has();
    }
    bool IsChild() const
    {
        return !parent.has();
    }
    Ref<WObject> GetOwner() const
    {
        return owner.lock();
    }
    Ref<WObject> GetParent() const
    {
        return parent.lock();
    }

    virtual void AddChild(Ref<WObject> child);
    virtual void RemoveChild(Ref<WObject> child);
    const std::vector<Ref<WObject>>& GetChildren() const
    {
        return children;
    }
    std::vector<Ref<WObject>> GetAllChildren() const
    {
        std::vector<Ref<WObject>> result = children;
        for (auto& obj : children)
        {
            auto t = obj->GetChildren();
            if (t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }

    void AddComponent(Ref<WObject> component);
    void RemoveComponent(Ref<WObject> component);
    const std::vector<Ref<WObject>>& GetComponents() const
    {
        return components;
    }
    std::vector<Ref<WObject>> GetAllComponents() const
    {
        std::vector<Ref<WObject>> result = components;
        for (auto& obj : components)
        {
            auto t = obj->GetComponents();
            if (t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }

    template<typename T>
    std::vector<Ref<T>> GetComponentsOfType()
    {
        std::vector<Ref<T>> result;
        for (auto& comp : components)
        {
            if (auto casted = RefCast<T>(comp))
            {
                result.push_back(casted);
            }
            std::vector<Ref<T>> t = comp->GetComponentsOfType<T>();
            if (t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }

    template<typename T>
    std::vector<Ref<T>> GetChildsOfType()
    {
        std::vector<Ref<T>> result;
        for (auto& child : children)
        {
            if (auto casted = RefCast<T>(child))
            {
                result.push_back(casted);
            }
            std::vector<Ref<T>> t = child->GetChildsOfType<T>();
            if (t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }

    TransformMode GetTransformMode() const
    {
        return transform_mode;
    }
    void SetTransformMode(TransformMode mode);

    void SetTransform(MMatrix4f transform);
    void SetPosition(MVector3f position);
    void SetRotation(MVector3f euler_angles);
    void SetRotation(MVector4f quaternion);
    void SetScale(MVector3f scale);

    void SetWorldTransform(MMatrix4f transform);
    void SetWorldPosition(MVector3f position);
    void SetWorldRotation(MVector3f euler_angles);
    void SetWorldRotation(MVector4f quaternion);
    void SetWorldScale(MVector3f scale);

    MMatrix4f GetTransform() const
    {
        return local_transform;
    }
    MVector3f GetPosition() const;
    MVector4f GetRotation() const;
    MVector3f GetScale() const;

    MMatrix4f GetWorldTransform() const
    {
        EnsureWorldTransform();
        return world_transform.Get();
    }
    MVector3f GetWorldPosition() const;
    MVector4f GetWorldRotation() const;
    MVector3f GetWorldScale() const;

    void BindWorldTransformStorage(MMatrix4f* ptr);

    void UnbindWorldTransformStorage();

    bool HasBoundWorldTransformStorage() const
    {
        return world_transform.IsExternal();
    }

    void ForceEvaluateWorldTransform() const
    {
        EnsureWorldTransform();
    }

    MMatrix4f ToLocal(const MMatrix4f& worldMatrix) const;
    MVector3f ToLocalPoint(const MVector3f& worldPoint) const;

    MMatrix4f ToWorld(const MMatrix4f& localMatrix) const;
    MVector3f ToWorldPoint(const MVector3f& localPoint) const;

    void AddTag(std::string tag, Data value = nullptr);
    void RemoveTag(std::string tag);
    bool HasTag(std::string tag) const;
    Data GetTagValue(std::string tag) const;
    const std::unordered_map<std::string, Data>& GetTags() const
    {
        return tags;
    }

    virtual void Update(float delta_time)
    {
    }
    void UpdateImp(float delta_time);

    bool IsAttachedTo(Ref<WObject> object) const
    {
        return IsAttachedToOwner(object) || IsAttachedToParent(object);
    };
    bool IsAttachedToOwner(Ref<WObject> potential_owner) const;
    bool IsAttachedToParent(Ref<WObject> potential_parent) const;

    void SetCanTick(const bool& val)
    {
        can_tick = val;
    }

private:
    friend class WComponent;
    friend class WActor;

    class TransformData
    {
    public:
        TransformData() = default;
        TransformData(const MMatrix4f& v) : inline_mat(v)
        {
        }

        MMatrix4f inline_mat = MMatrix4f::identity();
        MMatrix4f* external = nullptr;

        bool IsExternal() const
        {
            return external != nullptr;
        }

        MMatrix4f& Get()
        {
            return external ? *external : inline_mat;
        }
        const MMatrix4f& Get() const
        {
            return external ? *external : inline_mat;
        }

        MMatrix4f* Ptr()
        {
            return external ? external : &inline_mat;
        }
        const MMatrix4f* Ptr() const
        {
            return external ? external : &inline_mat;
        }

        void BindExternal(MMatrix4f* p)
        {
            if (external && external != p)
                inline_mat = *external;
            external = p;
            if (external)
                *external = inline_mat;
        }

        void UnbindExternal()
        {
            if (external)
            {
                inline_mat = *external;
                external = nullptr;
            }
        }

        operator MMatrix4f&()
        {
            return Get();
        }
        operator const MMatrix4f&() const
        {
            return Get();
        }

        TransformData& operator=(const MMatrix4f& v)
        {
            Get() = v;
            return *this;
        }
    };

    TransformMode transform_mode = TransformMode::Strong;
    MMatrix4f local_transform;
    mutable TransformData world_transform;
    mutable bool world_transform_dirty = true;

    std::vector<Ref<WObject>> children;
    std::vector<Ref<WObject>> components;
    std::unordered_map<std::string, Data> tags;
    WeakRef<WObject> owner = nullptr;
    WeakRef<WObject> parent = nullptr;

    std::string name;
    Ref<World> world = nullptr;

    bool can_tick = true;

    virtual void AttachTo(Ref<WObject> new_owner);
    virtual void SetParent(Ref<WObject> new_parent);
    void Detach();
    void OnTransformChanged();

    void MarkTransformDirty();
    void UpdateWorldTransformRecursive();
    void EnsureWorldTransform() const;
};

class WComponent : public WObject
{
public:
    WComponent() = delete;
    WComponent(Ref<WObject> owner, const std::string& name = "Component");
    bool IsComponent() const override
    {
        return true;
    }

    void AddChild(Ref<WObject> child) override
    {
        throw std::logic_error("Components cannot have children");
    }

protected:
    void InitializeAsComponent(Ref<WObject> owner);
};

class WActor : public WObject
{
public:
    WActor(const std::string& name = "Actor");
    bool IsComponent() const override
    {
        return false;
    }
    virtual void AttachTo(Ref<WObject> new_owner) override
    {
        throw std::logic_error("WActor cant be component");
    }
};