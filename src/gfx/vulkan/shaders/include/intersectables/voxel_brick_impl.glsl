#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL

#include <util.glsl>

#ifndef VOXEL_BRICK_IMPL_ARRAY
#error VOXEL_BRICK_IMPL_ARRAY must be defined
#endif //  VOXEL_BRICK_IMPL_ARRAY

// IntersectionResult
// VoxelBrick_tryIntersect2(const uint offset, const vec3 cornerPos, const Ray
// ray)
// {
//     Cube boundingCube;
//     boundingCube.center = cornerPos
//                         + vec3(
//                               VoxelBrick_EdgeLength / 2,
//                               VoxelBrick_EdgeLength / 2,
//                               VoxelBrick_EdgeLength / 2);
//     // +1 is for the fact that the voxels are centered at 0.5 offsets
//     boundingCube.edge_length = VoxelBrick_EdgeLength + 1;

//     ivec3 voxelStartIndexChecked;

//     if (!Cube_contains(boundingCube, ray.origin))
//     {
//         IntersectionResult result = Cube_tryIntersect(boundingCube, ray);

//         // The ray starts doesn't even hit this brick, it's a miss
//         if (!result.intersection_occurred)
//         {
//             return IntersectionResult_getMiss();
//         }
//         else // the brick does get hit by the ray
//         {
//             voxelStartIndexChecked =
//                 ivec3(result.maybe_hit_point - cornerPos + 0.5);
//         }
//     }
//     else
//     {
//         // voxelStartIndexChecked =
//         //     ivec3((ray.origin - cornerPos) / VoxelBrick_EdgeLength);
//         voxelStartIndexChecked =
//             ivec3((ray.origin - cornerPos) / VoxelBrick_EdgeLength + 0.5);
//     }

//     // if (Voxel_isVisible(thisVoxel))
//     // {
//     //     Sphere cube;
//     //     cube.center = voxelStartIndexChecked * 1.0 + cornerPos;
//     //     cube.radius = 0.5;

//     //     return Sphere_tryIntersect(cube, ray);
//     // }

//     // Assuming the following are defined:
//     vec3 rayDir    = ray.direction; // The direction of the ray
//     vec3 rayOrigin = ray.origin;    // The origin of the ray
//     vec3 voxelSize = vec3(1);       // The size of the voxel

//     vec3  tMax;
//     vec3  tDelta;
//     ivec3 step;
//     ivec3 voxel = voxelStartIndexChecked;

//     for (int i = 0; i < 3; ++i)
//     {
//         // Determine the voxel that the ray starts in
//         // voxel[i] = int(floor(rayOrigin[i] / voxelSize[i]));

//         // Determine the direction of the ray
//         if (rayDir[i] < 0)
//         {
//             step[i]   = -1;
//             tMax[i]   = (rayOrigin[i] - voxel[i] * voxelSize[i]) / rayDir[i];
//             tDelta[i] = voxelSize[i] / -rayDir[i];
//         }
//         else
//         {
//             step[i] = 1;
//             tMax[i] =
//                 ((voxel[i] + 1) * voxelSize[i] - rayOrigin[i]) / rayDir[i];
//             tDelta[i] = voxelSize[i] / rayDir[i];
//         }
//     }

//     while (true)
//     {
//         // Process voxel at voxel.x, voxel.y, voxel.z here
//         if (Voxel_isVisible(VOXEL_BRICK_IMPL_ARRAY[offset]
//                                 .voxels[voxel.x][voxel.y][voxel.z]))
//         {
//             // IntersectionResult result;
//             // result.intersection_occurred = true;
//             // result.maybe_distance        = 0.0;
//             // result.maybe_normal          = vec3(1);
//             // result.maybe_hit_point       = vec3(1);
//             // result.maybe_color           = vec4(1);
//             // // // result
//             Cube cube;
//             cube.center      = voxel * 1.0 + cornerPos;
//             cube.edge_length = 1.0;

//             return Cube_tryIntersect(cube, ray);
//         }

//         if (any(lessThan(voxel, ivec3(0))) || any(greaterThan(voxel,
//         ivec3(7))))
//         {
//             return IntersectionResult_getMiss();
//         }

//         // Move to next voxel
//         if (tMax.x < tMax.y)
//         {
//             if (tMax.x < tMax.z)
//             {
//                 voxel.x += step.x;
//                 tMax.x += tDelta.x;
//             }
//             else
//             {
//                 voxel.z += step.z;
//                 tMax.z += tDelta.z;
//             }
//         }
//         else
//         {
//             if (tMax.y < tMax.z)
//             {
//                 voxel.y += step.y;
//                 tMax.y += tDelta.y;
//             }
//             else
//             {
//                 voxel.z += step.z;
//                 tMax.z += tDelta.z;
//             }
//         }
//     }
// }

// IntersectionResult result;

// result.intersection_occurred = true;
// result.maybe_distance        = 0.0;
// result.maybe_hit_point       = ray.origin;
// result.maybe_normal          = vec3(0.0);
// result.maybe_color           = vec4(
//     vec3(voxelStartIndexChecked) / float(VoxelBrick_EdgeLength), 1.0);

// return result;

IntersectionResult
VoxelBrick_tryIntersect2(const uint offset, const vec3 cornerPos, const Ray ray)
{
    Cube boundingCube;
    boundingCube.center =
        cornerPos + vec3(VoxelBrick_EdgeLength / 2) - 0.5 * Voxel_Size;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    ivec3 voxelStartIndexChecked;
    {
        // the ray starts inside the cube, it's really easy to get the
        // coordinates of the ray
        if (Cube_contains(boundingCube, ray.origin))
        {
            voxelStartIndexChecked =
                ivec3(floor(ray.origin - cornerPos + 0.5 * Voxel_Size));
            // return IntersectionResult_getError();
        }
        else // we have to trace to the cube
        {
            IntersectionResult result = Cube_tryIntersect(boundingCube, ray);

            // this ray daesnt even hit this cube, exit
            if (!result.intersection_occurred)
            {
                return IntersectionResult_getMiss();
            }

            voxelStartIndexChecked = ivec3(
                result.maybe_hit_point - cornerPos + 0.5 * Voxel_Size
                - vec3(VERDIGRIS_EPSILON_MULTIPLIER * 10));
            // need an extra nudge down for the flooring to work right
        }
    }

    vec3  tMax;
    vec3  tDelta = vec3(Voxel_Size) / ray.direction;
    ivec3 step   = ivec3(sign(ray.direction) * Voxel_Size);
    ivec3 voxel  = voxelStartIndexChecked;

    for (int i = 0; i < 3; ++i)
    {
        if (ray.direction[i] < 0)
        {
            tMax[i] =
                (ray.origin[i] - voxel[i] * Voxel_Size) / ray.direction[i];
        }
        else
        {
            tMax[i] = ((voxel[i] + 1) * Voxel_Size - ray.origin[i])
                    / ray.direction[i];
        }
    }

    if (any(lessThan(voxel, ivec3(0))) || any(greaterThan(voxel, ivec3(7))))
    {
        return IntersectionResult_getError();
    }

    if (Voxel_isVisible(
            VOXEL_BRICK_IMPL_ARRAY[offset].voxels[voxel.x][voxel.y][voxel.z]))
    {
        Cube cube;
        cube.center      = voxel * Voxel_Size + cornerPos;
        cube.edge_length = Voxel_Size;

        IntersectionResult resu = Cube_tryIntersect(cube, ray);

        resu.maybe_color = Voxel_getLinearColor(
            VOXEL_BRICK_IMPL_ARRAY[offset].voxels[voxel.x][voxel.y][voxel.z]);

        if (resu.intersection_occurred)
        {
            return resu;
        }
        else
        {
            return IntersectionResult_getError();
        }
    }

    return IntersectionResult_getMiss();
}

IntersectionResult
VoxelBrick_tryIntersect3(const uint offset, const vec3 cornerPos, const Ray ray)
{
    // do we hit the box?
    Cube boundingCube;
    boundingCube.center =
        cornerPos + vec3(VoxelBrick_EdgeLength / 2) - 0.5f * Voxel_Size;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    ivec3 voxelStartIndexChecked;
    vec3  traversalStartPoint;
    {
        // the ray starts inside the cube, it's really easy to get the
        // coordinates of the ray
        if (Cube_contains(boundingCube, ray.origin))
        {
            voxelStartIndexChecked =
                ivec3(floor(ray.origin - cornerPos + 0.5f * Voxel_Size));
            // return IntersectionResult_getError();
            traversalStartPoint = ray.origin;
        }
        else // we have to trace to the cube
        {
            IntersectionResult result = Cube_tryIntersect(boundingCube, ray);

            // this ray daesnt even hit this cube, exit
            if (!result.intersection_occurred)
            {
                return IntersectionResult_getMiss();
            }

            voxelStartIndexChecked = ivec3(
                result.maybe_hit_point - cornerPos + 0.5f * Voxel_Size
                - vec3(VERDIGRIS_EPSILON_MULTIPLIER * 10));

            traversalStartPoint = result.maybe_hit_point;
            // need an extra nudge down for the flooring to work right
        }
    }

    vec3  tDelta       = vec3(Voxel_Size) / ray.direction;
    ivec3 step         = ivec3(sign(tDelta));
    ivec3 currentVoxel = voxelStartIndexChecked;
    vec3  tMax;
    for (int i = 0; i < 3; ++i)
    {
        const vec3  origin = traversalStartPoint;
        const float t1     = (floor(origin[i]) - origin[i]) / ray.direction[i];
        const float t2     = t1 + (Voxel_Size / ray.direction[i]);
        tMax[i]            = max(t1, t2);
    }
    int safetyBound = 0;

    do {
        if (Voxel_isVisible(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[currentVoxel.x][currentVoxel.y][currentVoxel.z]))
        {
            // IntersectionResult result;
            // result.intersection_occurred = true;
            // result.maybe_distance        = 0.0;
            // result.maybe_normal          = vec3(1);
            // result.maybe_hit_point       = vec3(1);
            // result.maybe_color           = vec4(1);
            // // // result
            Cube cube;
            cube.center      = currentVoxel * 1.0 + cornerPos;
            cube.edge_length = 1.0;

            IntersectionResult result = Cube_tryIntersect(cube, ray);

            result.maybe_color = Voxel_getLinearColor(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[currentVoxel.x][currentVoxel.y][currentVoxel.z]);

            if (result.intersection_occurred)
            {
                return result;
            }
            // else
            // {
            //     return IntersectionResult_getError();
            // }
        }

        // Move to next voxel
        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                currentVoxel.x += step.x;
                tMax.x += tDelta.x;
            }
            else
            {
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
        else
        {
            if (tMax.y < tMax.z)
            {
                currentVoxel.y += step.y;
                tMax.y += tDelta.y;
            }
            else
            {
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
    }
    while (all(greaterThanEqual(currentVoxel, ivec3(0)))
           && all(lessThanEqual(currentVoxel, ivec3(7))) && safetyBound++ < 50);

    if (safetyBound < 45)
    {
        return IntersectionResult_getMiss();
    }
    else
    {
        return IntersectionResult_getError();
    }
}

// if (Voxel_isVisible(
//         VOXEL_BRICK_IMPL_ARRAY[offset]
//             .voxels[voxelStartIndexChecked.x][voxelStartIndexChecked.y]
//                    [voxelStartIndexChecked.z]))
// {
//     Cube cube;

//     cube.center      = cornerPos + Voxel_Size * voxelStartIndexChecked;
//     cube.edge_length = Voxel_Size;

//     IntersectionResult result = Cube_tryIntersect(cube, ray);

//     return result;
// }
// else
// {
//     return IntersectionResult_getMiss();
// }
// }

IntersectionResult
VoxelBrick_tryIntersect(const uint offset, const vec3 cornerPos, Ray ray)
{
    Cube brickOuterCube;
    brickOuterCube.center = cornerPos
                          + vec3(
                                VoxelBrick_EdgeLength / 2,
                                VoxelBrick_EdgeLength / 2,
                                VoxelBrick_EdgeLength / 2);

    // +1 is for the fact that the voxels arent perfecetly ccentered
    brickOuterCube.edge_length = VoxelBrick_EdgeLength + 1;

    IntersectionResult result = IntersectionResult_getMiss();

    if (!Cube_tryIntersect(brickOuterCube, ray).intersection_occurred
        && !Cube_contains(brickOuterCube, ray.origin))
    {
        return IntersectionResult_getMiss();
    }

    for (uint i = 0; i < VoxelBrick_EdgeLength; ++i)
    {
        for (uint j = 0; j < VoxelBrick_EdgeLength; ++j)
        {
            for (uint k = 0; k < VoxelBrick_EdgeLength; ++k)
            {
                Voxel thisVoxel =
                    VOXEL_BRICK_IMPL_ARRAY[offset].voxels[i][j][k];

                if (Voxel_isVisible(thisVoxel))
                {
                    Cube cube;
                    cube.center      = vec3(i, j, k) * 1.0 + cornerPos;
                    cube.edge_length = 1.0;

                    IntersectionResult resu = Cube_tryIntersect(cube, ray);

                    resu.maybe_color = Voxel_getLinearColor(thisVoxel);

                    propagateIntersection(result, resu);
                }
            }
        }
    }

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL