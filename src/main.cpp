#include "gfx/object.hpp"
#include "glm/gtx/string_cast.hpp"
#include <future>
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <util/log.hpp>
#include <util/noise.hpp>

std::vector<glm::ivec3> voxelTraversal(glm::vec3 ray_start, glm::vec3 ray_end)
{
    std::vector<glm::ivec3> visited_voxels;
    const float             _bin_size = 1.0;

    // This id of the first/current voxel hit by the ray.
    // Using floor (round down) is actually very important,
    // the implicit int-casting will round up for negative numbers.
    glm::ivec3 current_voxel(
        std::floor(ray_start[0] / _bin_size),
        std::floor(ray_start[1] / _bin_size),
        std::floor(ray_start[2] / _bin_size));

    // The id of the last voxel hit by the ray.
    // TODO: what happens if the end point is on a border?
    glm::ivec3 last_voxel(
        std::floor(ray_end[0] / _bin_size),
        std::floor(ray_end[1] / _bin_size),
        std::floor(ray_end[2] / _bin_size));

    // Compute normalized ray direction.
    glm::vec3 ray = ray_end - ray_start;
    // ray           = glm::normalize(ray);

    // In which direction the voxel ids are incremented.
    double stepX = (ray[0] >= 0) ? 1 : -1; // correct
    double stepY = (ray[1] >= 0) ? 1 : -1; // correct
    double stepZ = (ray[2] >= 0) ? 1 : -1; // correct

    // Distance along the ray to the next voxel border from the current position
    // (tMaxX, tMaxY, tMaxZ).
    double next_voxel_boundary_x =
        (current_voxel[0] + stepX) * _bin_size; // correct
    double next_voxel_boundary_y =
        (current_voxel[1] + stepY) * _bin_size; // correct
    double next_voxel_boundary_z =
        (current_voxel[2] + stepZ) * _bin_size; // correct

    // tMaxX, tMaxY, tMaxZ -- distance until next intersection with voxel-border
    // the value of t at which the ray crosses the first vertical voxel boundary
    double tMaxX = (ray[0] != 0)
                     ? (next_voxel_boundary_x - ray_start[0]) / ray[0]
                     : DBL_MAX; //
    double tMaxY = (ray[1] != 0)
                     ? (next_voxel_boundary_y - ray_start[1]) / ray[1]
                     : DBL_MAX; //
    double tMaxZ = (ray[2] != 0)
                     ? (next_voxel_boundary_z - ray_start[2]) / ray[2]
                     : DBL_MAX; //

    // tDeltaX, tDeltaY, tDeltaZ --
    // how far along the ray we must move for the horizontal component to equal
    // the width of a voxel the direction in which we traverse the grid can only
    // be FLT_MAX if we never go in that direction
    double tDeltaX = (ray[0] != 0) ? _bin_size / ray[0] * stepX : DBL_MAX;
    double tDeltaY = (ray[1] != 0) ? _bin_size / ray[1] * stepY : DBL_MAX;
    double tDeltaZ = (ray[2] != 0) ? _bin_size / ray[2] * stepZ : DBL_MAX;

    glm::ivec3 diff(0, 0, 0);
    bool       neg_ray = false;
    if (current_voxel[0] != last_voxel[0] && ray[0] < 0)
    {
        diff[0]--;
        neg_ray = true;
    }
    if (current_voxel[1] != last_voxel[1] && ray[1] < 0)
    {
        diff[1]--;
        neg_ray = true;
    }
    if (current_voxel[2] != last_voxel[2] && ray[2] < 0)
    {
        diff[2]--;
        neg_ray = true;
    }
    visited_voxels.push_back(current_voxel);
    if (neg_ray)
    {
        current_voxel += diff;
        visited_voxels.push_back(current_voxel);
    }

    while (last_voxel != current_voxel)
    {
        if (tMaxX < tMaxY)
        {
            if (tMaxX < tMaxZ)
            {
                current_voxel[0] += stepX;
                tMaxX += tDeltaX;
            }
            else
            {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        else
        {
            if (tMaxY < tMaxZ)
            {
                current_voxel[1] += stepY;
                tMaxY += tDeltaY;
            }
            else
            {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        visited_voxels.push_back(current_voxel);
    }
    return visited_voxels;
}

std::vector<glm::ivec3>
voxelTraversal2(glm::vec3 ray_start, glm::vec3 ray_direction)
{
    ray_direction = glm::normalize(ray_direction);

    std::vector<glm::ivec3> visited_voxels;
    const float             _bin_size = 1.0;

    // This id of the first/current voxel hit by the ray.
    // Using floor (round down) is actually very important,
    // the implicit int-casting will round up for negative numbers.
    glm::ivec3 current_voxel(
        std::floor(ray_start[0] / _bin_size),
        std::floor(ray_start[1] / _bin_size),
        std::floor(ray_start[2] / _bin_size));

    // The id of the last voxel hit by the ray.
    // TODO: what happens if the end point is on a border?

    // Compute normalized ray direction.
    // glm::vec3 ray        = ray_end - ray_start;
    // glm::vec3 ray_origin = ray_start;
    // ray.normalize();

    // In which direction the voxel ids are incremented.
    float stepX = (ray_direction[0] >= 0) ? 1 : -1; // correct
    float stepY = (ray_direction[1] >= 0) ? 1 : -1; // correct
    float stepZ = (ray_direction[2] >= 0) ? 1 : -1; // correct

    // Distance along the ray to the next voxel border from the current position
    // (tMaxX, tMaxY, tMaxZ).
    float next_voxel_boundary_x =
        (current_voxel[0] + stepX) * _bin_size; // correct
    float next_voxel_boundary_y =
        (current_voxel[1] + stepY) * _bin_size; // correct
    float next_voxel_boundary_z =
        (current_voxel[2] + stepZ) * _bin_size; // correct

    // tMaxX, tMaxY, tMaxZ -- distance until next intersection with voxel-border
    // the value of t at which the ray crosses the first vertical voxel boundary
    float tMaxX = (ray_direction[0] != 0)
                    ? (next_voxel_boundary_x - ray_start[0]) / ray_direction[0]
                    : FLT_MAX; //
    float tMaxY = (ray_direction[1] != 0)
                    ? (next_voxel_boundary_y - ray_start[1]) / ray_direction[1]
                    : FLT_MAX; //
    float tMaxZ = (ray_direction[2] != 0)
                    ? (next_voxel_boundary_z - ray_start[2]) / ray_direction[2]
                    : FLT_MAX; //

    // tDeltaX, tDeltaY, tDeltaZ --
    // how far along the ray we must move for the horizontal component to equal
    // the width of a voxel the direction in which we traverse the grid can only
    // be FLT_MAX if we never go in that direction
    float tDeltaX = (ray_direction[0] != 0)
                      ? _bin_size / ray_direction[0] * stepX
                      : FLT_MAX;
    float tDeltaY = (ray_direction[1] != 0)
                      ? _bin_size / ray_direction[1] * stepY
                      : FLT_MAX;
    float tDeltaZ = (ray_direction[2] != 0)
                      ? _bin_size / ray_direction[2] * stepZ
                      : FLT_MAX;

    glm::ivec3 diff(0, 0, 0);
    bool       neg_ray = false;
    if (/* current_voxel[0] != last_voxel[0] && */ ray_direction[0] < 0)
    {
        diff[0]--;
        neg_ray = true;
    }
    if (/* current_voxel[1] != last_voxel[1] && */ ray_direction[1] < 0)
    {
        diff[1]--;
        neg_ray = true;
    }
    if (/* current_voxel[2] != last_voxel[2] && */ ray_direction[2] < 0)
    {
        diff[2]--;
        neg_ray = true;
    }
    visited_voxels.push_back(current_voxel);
    if (neg_ray)
    {
        current_voxel += diff;
        visited_voxels.push_back(current_voxel);
    }

    while (glm::all(glm::greaterThanEqual(current_voxel, glm::ivec3 {0}))
           && glm::all(glm::lessThanEqual(current_voxel, glm::ivec3 {7})))
    {
        if (tMaxX < tMaxY)
        {
            if (tMaxX < tMaxZ)
            {
                current_voxel[0] += stepX;
                tMaxX += tDeltaX;
            }
            else
            {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        else
        {
            if (tMaxY < tMaxZ)
            {
                current_voxel[1] += stepY;
                tMaxY += tDeltaY;
            }
            else
            {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        visited_voxels.push_back(current_voxel);
    }
    return visited_voxels;
}

int main()
{
    util::installGlobalLoggerRacy();

#ifdef NDEBUG
    util::setLoggingLevel(util::LoggingLevel::Log);
#else
    util::setLoggingLevel(util::LoggingLevel::Trace); // TODO: commandline args
#endif

    try
    {
        std::vector<glm::ivec3> data = voxelTraversal({0, 0, 0}, {3, 2, 2});
        glm::vec3               dir = glm::vec3 {3, 2, 2} - glm::vec3 {0, 0, 0};
        std::vector<glm::ivec3> data2 = voxelTraversal2({0, 0, 0}, dir);

        for (std::size_t i = 0; i < data.size(); ++i)
        {
            auto l    = glm::to_string(data[i]);
            auto r    = glm::to_string(data2[i]);
            bool pass = (l == r);

            util::logLog("Traversed {} | {} | {}", l, r, pass);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);

            // if (!pass)
            // {
            //     util::panic("fail");
            // }
        }

        util::panic("end test");

        gfx::Renderer renderer {};
        game::Game    game {renderer};

        std::atomic<bool> shouldStop {false};

        std::future<void> gameLoop = std::async(
            [&]
            {
                while (game.continueTicking()
                       && !shouldStop.load(std::memory_order_acquire))
                {
                    game.tick();
                }

                shouldStop.store(true, std::memory_order_release);
            });

        while (renderer.continueTicking()
               && !shouldStop.load(std::memory_order_acquire))
        {
            renderer.drawFrame();
        }

        shouldStop.store(true, std::memory_order_release);

        gameLoop.wait();
    }
    catch (const std::exception& e)
    {
        util::logFatal("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}