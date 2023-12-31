#ifndef SRC_GAME_ENTITY_CUBE_HPP
#define SRC_GAME_ENTITY_CUBE_HPP

#include "entity.hpp"
#include <memory>

namespace gfx
{
    class Renderer;
    class SimpleTriangulatedObject;
} // namespace gfx

namespace game::entity
{
    class Cube final : public Entity
    {
    public:

        static std::shared_ptr<Cube> create(const Game&, glm::vec3 position);
        ~Cube() override = default;

        void tick() const override;

    private:
        std::shared_ptr<gfx::SimpleTriangulatedObject> object;

        glm::vec3     root;
        mutable float time_alive;

        Cube(const Game&, glm::vec3 position);
    };
} // namespace game::entity

#endif // SRC_GAME_ENTITY_CUBE_HPP
