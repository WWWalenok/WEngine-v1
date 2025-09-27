#pragma once
#include "WObject.h"

class Camera : public WObject {
public:
    Camera(std::string name = "Camera");
    
    void SetTarget(MVector3f target);
    MMatrix4f GetViewMatrix() const;
    MMatrix4f GetProjectionMatrix(float aspect) const;
    
    void Update(float delta_time) override;
    
    MVector3f target_position = {0.0f, 0.0f, 0.0f};
    float fov = 60.0f;
    float near_clip = 0.1f;
    float far_clip = 1000.0f;
};

DECLARE_DATA_TYPE(Camera);