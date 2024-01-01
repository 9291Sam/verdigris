#include "player.hpp"
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

namespace game
{
    Player::Player(const Game& game_, glm::vec3 position)
        : game {game_}
        , camera {position}
    {}

    void Player::tick()
    {
        // TODO: moving diaginally is faster
        const float MoveScale        = this->game.renderer.isActionActive(
                                    gfx::Window::Action::PlayerSprint)
                                         ? 25.0f
                                         : 10.0f;
        const float rotateSpeedScale = 1.0f;

        this->camera.addPosition(
            this->camera.getForwardVector()
            * this->game.getTickDeltaTimeSeconds() * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveForward)
                   ? 1.0f
                   : 0.0f));

        this->camera.addPosition(
            -this->camera.getForwardVector()
            * this->game.getTickDeltaTimeSeconds() * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveBackward)
                   ? 1.0f
                   : 0.0f));

        this->camera.addPosition(
            -this->camera.getRightVector()
            * this->game.getTickDeltaTimeSeconds() * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveLeft)
                   ? 1.0f
                   : 0.0f));

        this->camera.addPosition(
            this->camera.getRightVector() * this->game.getTickDeltaTimeSeconds()
            * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveRight)
                   ? 1.0f
                   : 0.0f));

        this->camera.addPosition(
            gfx::Transform::UpVector * this->game.getTickDeltaTimeSeconds()
            * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveUp)
                   ? 1.0f
                   : 0.0f));

        this->camera.addPosition(
            -gfx::Transform::UpVector * this->game.getTickDeltaTimeSeconds()
            * MoveScale
            * (this->game.renderer.isActionActive(
                   gfx::Window::Action::PlayerMoveDown)
                   ? 1.0f
                   : 0.0f));

        auto [xDelta, yDelta] = this->game.renderer.getMouseDeltaRadians();

        // huh?
        this->camera.addYaw(
            xDelta * rotateSpeedScale * this->game.getTickDeltaTimeSeconds()
            / this->game.renderer.getFrameDeltaTimeSeconds());
        this->camera.addPitch(
            yDelta * rotateSpeedScale * this->game.getTickDeltaTimeSeconds()
            / this->game.renderer.getFrameDeltaTimeSeconds());

        // this->game.renderer.getMenu().setPlayerPosition(
        //     this->camera.getPosition());
    }

    gfx::Camera& Player::getCamera()
    {
        return this->camera;
    }

} // namespace game
