#pragma once
#include "WObject.h"

class WComponent : public WObject {
public:
    WComponent() = delete;
    WComponent(WObject* owner, std::string name = "Component");
    bool IsComponent() const override { return true; }
    
    void AddChild(Ref<WObject> child) override {
        throw std::logic_error("Components cannot have children");
    }
    
protected:
    void InitializeAsComponent(WObject* owner);
};

DECLARE_DATA_TYPE(WComponent);