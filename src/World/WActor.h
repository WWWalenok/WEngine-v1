#pragma once
#include "WObject.h"

class WActor : public WObject {
public:
    WActor(std::string name = "Actor");
    virtual void AttachTo(WObject* new_owner) override { throw std::logic_error("WActor cant be component"); }
};

DECLARE_DATA_TYPE(WActor);