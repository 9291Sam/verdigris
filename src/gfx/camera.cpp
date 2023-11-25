#include "camera.hpp"
#include "gfx/transform.hpp"
#include "renderer.hpp"
#include <fmt/format.h>
#include <util/log.hpp>

namespace gfx
{
    Camera::Camera(glm::vec3 position)
        : pitch {0.0f}
        , yaw {0.0f} // clang-format off
        , transform {
            .translation {position},
            .rotation {1.0f, 0.0f, 0.0f, 0.0f},
            .scale {1.0f, 1.0f, 1.0f}}
    // clang-format on
    {}

    glm::mat4 Camera::getPerspectiveMatrix(
        const Renderer& renderer, const Transform& transform) const
    {
        // TODO: replace
        // https://gist.github.com/pezcode/1609b61a1eedd207ec8c5acf6f94f53a
        glm::mat4 projection = glm::perspective(
            renderer.getFovYRadians(),
            renderer.getAspectRatio(),
            // NOTE: these are hardcoded into uber_voxel.comp, do not change
            0.1f,       // NOLINT
            100000.0f); // NOLINT

        // glm is designed for openGL, vulkan's Y coordinate is flipped in
        // comparason
        projection[1][1] *= -1;

        return projection * this->getViewMatrix() * transform.asModelMatrix();
    }

    glm::mat4 Camera::getViewMatrix() const
    {
        return glm::inverse(
            this->transform.asTranslationMatrix()
            * this->transform.asRotationMatrix());
    }

    glm::vec3 Camera::getForwardVector() const
    {
        return this->transform.getForwardVector();
    }

    glm::vec3 Camera::getRightVector() const
    {
        return this->transform.getRightVector();
    }

    glm::vec3 Camera::getUpVector() const
    {
        return this->transform.getUpVector();
    }

    glm::vec3 Camera::getPosition() const
    {
        return this->transform.translation;
    }

    void Camera::addPosition(glm::vec3 positionToAdd)
    {
        this->transform.translation += positionToAdd;
    }

    void Camera::addPitch(float pitchToAdd)
    {
        this->pitch += pitchToAdd;
        this->updateTransformFromRotations();
        // const float pitchBefore =
        // glm::eulerAngles(this->transform.rotation).p; if (pitchBefore +
        // pitchToAdd > glm::half_pi<float>())
        // {
        //     this->transform.pitchBy(glm::half_pi<float>() - pitchBefore);

        //     util::logTrace("Yawing by {:.8f}", yawToAdd);
        // }
        // else
        // {
        //     this->transform.pitchBy(pitchToAdd);
        // }
    }

    void Camera::addYaw(float yawToAdd)
    {
        this->yaw += yawToAdd;
        this->updateTransformFromRotations();
    }

    Camera::operator std::string () const
    {
        return fmt::format(
            "{}Pitch {} | Yaw {}",
            static_cast<std::string>(this->transform),
            this->pitch,
            this->yaw);
    }

    void Camera::updateTransformFromRotations()
    {
        glm::quat q {1.0f, 0.0f, 0.0f, 0.0f};

        this->pitch = std::clamp(
            this->pitch, -glm::half_pi<float>(), glm::half_pi<float>());
        this->yaw = glm::mod(this->yaw, glm::two_pi<float>());

        q *= glm::angleAxis(this->pitch, -Transform::RightVector);

        q = glm::angleAxis(this->yaw, -Transform::UpVector) * q;

        this->transform.rotation = q;
    }
} // namespace gfx
