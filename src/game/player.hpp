#ifndef SRC_GAME_PLAYER_HPP
#define SRC_GAME_PLAYER_HPP

#include <gfx/camera.hpp>

namespace gfx
{
    class Renderer;
}

namespace game
{
    class Game;

    class Player
    {
    public:

        Player(const Game&, glm::vec3 position);
        ~Player() = default;

        Player(const Player&)             = delete;
        Player(Player&&)                  = delete;
        Player& operator= (const Player&) = delete;
        Player& operator= (Player&&)      = delete;

        void         tick();
        gfx::Camera& getCamera();

    private:
        const Game& game;
        gfx::Camera camera;
    };
} // namespace game

#endif // SRC_GAME_PLAYER_HPP
