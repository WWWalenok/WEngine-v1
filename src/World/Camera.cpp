#include "Camera.h"

Camera::Camera(std::string name) : WObject(std::move(name)) {}

void Camera::SetTarget(MVector3f target) {
    auto position = GetWorldPosition();
    MVector3f forward = (target - position).GetNormalized();
    MVector3f worldUp(0.0f, 0.0f, 1.0f);
    MVector3f right = (forward / worldUp).GetNormalized();   // cross
    MVector3f up    = (right / forward).GetNormalized();     // cross

    MMatrix4f viewMatrix = wm::LookAt(position, target, up);
    SetWorldTransform(viewMatrix.invert());
}

MMatrix4f Camera::GetViewMatrix() const {
    return GetWorldTransform().invert();
}

MMatrix4f Camera::GetProjectionMatrix(float aspect) const {
    return wm::Perspective(fov * 0.0174532925199432957692369, aspect, near_clip, far_clip);
}

void Camera::Update(float delta_time) {
    WObject::Update(delta_time);
}