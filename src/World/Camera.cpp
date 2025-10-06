#include "Camera.h"

Camera::Camera(std::string name) : WObject(std::move(name)) {}

void Camera::SetTarget(MVector3f target) {
    target_position = target;
}

MMatrix4f Camera::GetViewMatrix() const {
    auto position = GetPosition();
    auto forward = (target_position - position);
    forward = forward / !forward;
    auto right = MVector3f(0.0f, 0.0f, 1.0f) / forward;
    right = right / !right;
    auto up = forward / right;
    up = up / !up;
    
    return wm::LookAt(position, target_position, up);
}

MMatrix4f Camera::GetProjectionMatrix(float aspect) const {
    return wm::Perspective(fov * 0.0174532925199432957692369, aspect, near_clip, far_clip);
}

void Camera::Update(float delta_time) {
    WObject::Update(delta_time);
}