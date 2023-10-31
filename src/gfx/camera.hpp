#ifndef SRC_GFX_CAMERA_HPP
#define SRC_GFX_CAMERA_HPP

#include "transform.hpp"

namespace gfx
{
    class Renderer;

    class Camera
    {
    public:

        explicit Camera(glm::vec3 position);

        [[nodiscard]] glm::mat4
        getPerspectiveMatrix(const Renderer&, const Transform&) const;
        [[nodiscard]] glm::mat4 getViewMatrix() const;
        [[nodiscard]] glm::vec3 getForwardVector() const;
        [[nodiscard]] glm::vec3 getRightVector() const;
        [[nodiscard]] glm::vec3 getUpVector() const;
        [[nodiscard]] glm::vec3 getPosition() const;

        void addPosition(glm::vec3);
        void addPitch(float);
        void addYaw(float);

        explicit operator std::string () const;

    private:
        void updateTransformFromRotations();

        float pitch;
        float yaw;

        Transform transform;
    };
} // namespace gfx

#endif // SRC_GfX_CAMERA_HPP
