#ifndef SRC_GAME_ENTITY_DISK__ENTITY_HPP
#define SRC_GAME_ENTITY_DISK__ENTITY_HPP

#include "entity.hpp"
#include <memory>

namespace gfx
{
    class Renderer;
    class Object;
    class SimpleTriangulatedObject;
} // namespace gfx

namespace game::entity
{
    class DiskEntity final : public Entity
    {
    public:

        static std::shared_ptr<DiskEntity>
        create(const Game& game, const char* filepath);
        ~DiskEntity() override;

        void tick() const override;

    private:
        std::shared_ptr<gfx::SimpleTriangulatedObject> object;

        DiskEntity(const Game&, const char* filepath);
    }; // class DiskEntity

} // namespace game::entity

#endif // SRC_GAME_ENTITY_DISK__ENTITY_HPP
