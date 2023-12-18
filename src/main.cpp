#include "gfx/object.hpp"
#include <future>
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>
#include <util/log.hpp>
#include <util/noise.hpp>

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

glm::vec3 Ray_at(Ray self, float t)
{
    return self.origin + self.direction * t;
}

struct IntersectionResult
{
    bool      intersection_occurred;
    float     maybe_distance;
    glm::vec3 maybe_hit_point;
    glm::vec3 maybe_normal;
    glm::vec4 maybe_color;
};

IntersectionResult IntersectionResult_getMiss()
{
    IntersectionResult result {};
    result.intersection_occurred = false;

    return result;
}

IntersectionResult IntersectionResult_getError()
{
    IntersectionResult result {};

    result.intersection_occurred = true;
    result.maybe_distance        = -1.0;
    result.maybe_hit_point       = glm::vec3(0.0);
    result.maybe_normal          = glm::vec3(1.0, 0.0, 1.0);
    result.maybe_color           = glm::vec4(1.0, 0.0, 1.0, 1.0);

    return result;
}

#define VERDIGRIS_EPSILON_MULTIPLIER 0.00001f

bool isApproxEqual(const float a, const float b)
{
    const float maxMagnitude = std::fmax(abs(a), abs(b));
    const float epsilon      = maxMagnitude * VERDIGRIS_EPSILON_MULTIPLIER;

    return abs(a - b) < epsilon;
}

float maxComponent(const glm::vec3 vec)
{
    return std::fmax(vec.x, std::fmax(vec.y, vec.z));
}

float minComponent(const glm::vec3 vec)
{
    return std::fmin(vec.x, std::fmin(vec.y, vec.z));
}

struct Cube
{
    glm::vec3 center;
    float     edge_length;
};

bool Cube_contains(const Cube self, const glm::vec3 point)
{
    const glm::vec3 p0 = self.center - (self.edge_length / 2);
    const glm::vec3 p1 = self.center + (self.edge_length / 2);

    if (all(lessThan(p0, point)) && all(lessThan(point, p1))) // NOLINT
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Stolen from
// https://studenttheses.uu.nl/bitstream/handle/20.500.12932/20460/final.pdf
// Appendix A Ray-Box intersection algorithm
IntersectionResult Cube_tryIntersect(const Cube self, const Ray ray)
{
    const glm::vec3 p0 = self.center - (self.edge_length / 2);
    const glm::vec3 p1 = self.center + (self.edge_length / 2);

    // ray starts inside the cube, we want nothing rendererd
    if (Cube_contains(self, ray.origin))
    {
        return IntersectionResult_getMiss();
    }

    const glm::vec3 t1 = (p0 - ray.origin) / ray.direction;
    const glm::vec3 t2 = (p1 - ray.origin) / ray.direction;

    const glm::vec3 vMax = max(t1, t2);
    const glm::vec3 vMin = min(t1, t2);

    const float tMax = minComponent(vMax);
    const float tMin = maxComponent(vMin);

    const bool hit = (tMin <= tMax && tMax > 0);

    if (!hit)
    {
        return IntersectionResult_getMiss();
    }

    const glm::vec3 hitPoint = Ray_at(ray, tMin);

    IntersectionResult result;
    result.intersection_occurred = true;
    result.maybe_distance        = length(ray.origin - hitPoint);
    result.maybe_hit_point       = hitPoint;

    glm::vec3 normal;

    if (isApproxEqual(hitPoint.x, p1.x))
    {
        normal = glm::vec3(-1.0, 0.0, 0.0); // Hit right face
    }
    else if (isApproxEqual(hitPoint.x, p0.x))
    {
        normal = glm::vec3(1.0, 0.0, 0.0); // Hit left face
    }
    else if (isApproxEqual(hitPoint.y, p1.y))
    {
        normal = glm::vec3(0.0, -1.0, 0.0); // Hit top face
    }
    else if (isApproxEqual(hitPoint.y, p0.y))
    {
        normal = glm::vec3(0.0, 1.0, 0.0); // Hit bottom face
    }
    else if (isApproxEqual(hitPoint.z, p1.z))
    {
        normal = glm::vec3(0.0, 0.0, -1.0); // Hit front face
    }
    else if (isApproxEqual(hitPoint.z, p0.z))
    {
        normal = glm::vec3(0.0, 0.0, 1.0); // Hit back face
    }
    else
    {
        normal = glm::vec3(0.0, 0.0, 0.0); // null case
    }

    result.maybe_normal = normal;
    result.maybe_color  = glm::vec4(0.0, 1.0, 1.0, 1.0);

    return result;
}

const float Voxel_Size            = 1.0;
const float VoxelBrick_EdgeLength = 8.0f;

glm::ivec3 getGridCord(glm::vec3 point)
{
    return glm::ivec3 {floor(point + 0.5f * Voxel_Size)};
}

std::vector<glm::ivec3> traverse(Ray ray, glm::vec3 cornerPos)
{
    const std::int64_t voxelExtent = 8; // 0 - 7

    // do we hit the box?
    Cube boundingCube;
    boundingCube.center =
        cornerPos + glm::vec3(VoxelBrick_EdgeLength / 2) - 0.5f * Voxel_Size;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    glm::ivec3 voxelStartIndexChecked;
    {
        // the ray starts inside the cube, it's really easy to get the
        // coordinates of the ray
        if (Cube_contains(boundingCube, ray.origin))
        {
            voxelStartIndexChecked =
                glm::ivec3(floor(ray.origin - cornerPos + 0.5f * Voxel_Size));
            // return IntersectionResult_getError();
        }
        else // we have to trace to the cube
        {
            IntersectionResult result = Cube_tryIntersect(boundingCube, ray);

            // this ray daesnt even hit this cube, exit
            if (!result.intersection_occurred)
            {
                return std::vector<glm::ivec3> {};
            }

            voxelStartIndexChecked = glm::ivec3(
                result.maybe_hit_point - cornerPos + 0.5f * Voxel_Size
                - glm::vec3(VERDIGRIS_EPSILON_MULTIPLIER * 10));
            // need an extra nudge down for the flooring to work right
        }
    }

    std::vector<glm::ivec3> traversedVoxels {};
    traversedVoxels.push_back(voxelStartIndexChecked);

    glm::vec3 tDelta = glm::vec3 {Voxel_Size} / ray.direction;
    util::logLog(
        "Corner: {} | tDelta: {}",
        glm::to_string(cornerPos),
        glm::to_string(tDelta));

    // get starting coord

    // make new traversal ray

    // traverse

    return traversedVoxels;
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
        {
            auto in  = glm::vec3 {-1.2, -1.2, -1.2};
            auto res = glm::ivec3 {-1, -1, -1};
            auto out = getGridCord(in);

            util::assertFatal(
                res == out,
                "Failed! {} | {}",
                glm::to_string(in),
                glm::to_string(out));
        }

        {
            auto in  = glm::vec3 {0.6, 0.6, 0.4};
            auto res = glm::ivec3 {1, 1, 0};
            auto out = getGridCord(in);

            util::assertFatal(
                res == out,
                "Failed! {} | {}",
                glm::to_string(in),
                glm::to_string(out));
        }

        {
            auto in  = glm::vec3 {0.6, -0.6, -0.4};
            auto res = glm::ivec3 {1, -1, 0};
            auto out = getGridCord(in);

            util::assertFatal(
                res == out,
                "Failed! {} | {}",
                glm::to_string(in),
                glm::to_string(out));
        }

        // miss
        {
            Ray ray {
                {10.0f, 0.0f, 0.0f},
                glm::normalize(glm::vec3 {3.0f, 2.0f, 2.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});

            util::assertFatal(res.empty(), "vector was not empty");
        }

        // close miss 0
        {
            Ray ray {
                {-0.6f, -0.6f, -0.6f},
                glm::normalize(glm::vec3 {-1.0f, -1.0f, -1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});

            util::assertFatal(res.empty(), "vector was not empty");
        }

        // close miss +
        {
            Ray ray {
                {7.51f, 7.51f, 7.51f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});

            util::assertFatal(res.empty(), "vector was not empty");
        }

        // close miss 0
        {
            Ray ray {
                {-0.6f, -0.6f, -0.6f},
                glm::normalize(glm::vec3 {-1.0f, -1.0f, -1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});

            util::assertFatal(res.empty(), "vector was not empty");
        }

        // close miss +
        {
            Ray ray {
                {7.51f, 7.51f, 7.51f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});

            util::assertFatal(res.empty(), "vector was not empty");
        }

        // hit internal
        {
            Ray ray {
                {7.49f, 7.49f, 7.49f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {0.0f, 0.0f, 0.0f});
            std::vector<glm::ivec3> dsi {glm::ivec3 {7, 7, 7}};

            util::assertFatal(res.size() == dsi.size(), "size mismatch");
            for (std::size_t i = 0; i < res.size(); ++i)
            {
                util::assertFatal(
                    res[i] == dsi[i],
                    "mismatch {} {}",
                    glm::to_string(res[i]),
                    glm::to_string(dsi[i]));
            }
        }

        // hit internal offset +
        {
            Ray ray {
                {23.49f, 23.49f, 23.49f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {16.0f, 16.0f, 16.0f});
            std::vector<glm::ivec3> dsi {glm::ivec3 {7, 7, 7}};

            util::assertFatal(res.size() == dsi.size(), "size mismatch");
            for (std::size_t i = 0; i < res.size(); ++i)
            {
                util::assertFatal(
                    res[i] == dsi[i],
                    "mismatch {} {}",
                    glm::to_string(res[i]),
                    glm::to_string(dsi[i]));
            }
        }

        // hit internal offset -
        {
            Ray ray {
                {15.51f, 15.51f, 15.51f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {16.0f, 16.0f, 16.0f});
            std::vector<glm::ivec3> dsi {glm::ivec3 {0, 0, 0}};

            util::assertFatal(res.size() == dsi.size(), "size mismatch");
            for (std::size_t i = 0; i < res.size(); ++i)
            {
                util::assertFatal(
                    res[i] == dsi[i],
                    "mismatch {} {}",
                    glm::to_string(res[i]),
                    glm::to_string(dsi[i]));
            }
        }

        // hit external offset -
        {
            Ray ray {
                {15.49f, 15.49f, 15.49f},
                glm::normalize(glm::vec3 {1.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {16.0f, 16.0f, 16.0f});
            std::vector<glm::ivec3> dsi {glm::ivec3 {0, 0, 0}};

            util::assertFatal(res.size() == dsi.size(), "size mismatch");
            for (std::size_t i = 0; i < res.size(); ++i)
            {
                util::assertFatal(
                    res[i] == dsi[i],
                    "mismatch {} {}",
                    glm::to_string(res[i]),
                    glm::to_string(dsi[i]));
            }
        }

        // hit external close -
        {
            Ray ray {
                {14.49f, 15.49f, 15.49f},
                glm::normalize(glm::vec3 {100.0f, 1.0f, 1.0f})};

            std::vector<glm::ivec3> res = traverse(ray, {16.0f, 16.0f, 16.0f});
            std::vector<glm::ivec3> dsi {glm::ivec3 {0, 0, 0}};

            util::assertFatal(res.size() == dsi.size(), "size mismatch");
            for (std::size_t i = 0; i < res.size(); ++i)
            {
                util::assertFatal(
                    res[i] == dsi[i],
                    "mismatch {} {}",
                    glm::to_string(res[i]),
                    glm::to_string(dsi[i]));
            }
        }

        // hit external
        // hit on axies internal / external.
        // two each of close hit and close miss
        // one in the middle

        util::panic("tests passed");

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