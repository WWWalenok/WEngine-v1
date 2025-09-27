#include "WComponent.h"
#include "WObject.h"

WComponent::WComponent(WObject* owner, std::string name)
    : WObject(std::move(name)) 
{
    if (!owner) {
        throw std::invalid_argument("Component must have an owner");
    }
    InitializeAsComponent(owner);
}

void WComponent::InitializeAsComponent(WObject* owner) {
    owner->AddComponent(this);
}