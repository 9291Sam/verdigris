#include "transform.hpp"
#include <format>
#include <glm/gtx/string_cast.hpp>

namespace gfx
{
    glm::mat4 Transform::asModelMatrix() const
    {
        return this->asTranslationMatrix() * this->asRotationMatrix()
             * this->asScaleMatrix();
    }

    glm::mat4 Transform::asTranslationMatrix() const
    {
        return glm::translate(glm::mat4 {1.0f}, this->translation);
    }

    glm::mat4 Transform::asRotationMatrix() const
    {
        return glm::mat4_cast(this->rotation);
    }

    glm::mat4 Transform::asScaleMatrix() const
    {
        return glm::scale(glm::mat4 {1.0f}, this->scale);
    }

    glm::vec3 Transform::getForwardVector() const
    {
        return this->rotation * Transform::ForwardVector;
    }

    glm::vec3 Transform::getUpVector() const
    {
        return this->rotation * Transform::UpVector;
    }

    glm::vec3 Transform::getRightVector() const
    {
        return this->rotation * Transform::RightVector;
    }

    void Transform::pitchBy(float pitch)
    {
        this->rotation =
            glm::rotate(this->rotation, pitch, Transform::RightVector);
    }

    void Transform::yawBy(float yaw)
    {
        this->rotation = glm::rotate(this->rotation, yaw, Transform::UpVector);
    }

    void Transform::rollBy(float roll)
    {
        this->rotation =
            glm::rotate(this->rotation, roll, Transform::ForwardVector);
    }

    Transform::operator std::string () const
    {
        return std::format(
            "Transform:\nPosition: {}\nRotation : {}\nScale:{}\n",
            glm::to_string(this->translation),
            glm::to_string(this->rotation),
            glm::to_string(this->scale));
    }

} // namespace gfx
