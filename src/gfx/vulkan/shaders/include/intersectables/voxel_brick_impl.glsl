#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL

#include <util.glsl>

#ifndef VOXEL_BRICK_IMPL_ARRAY
#error VOXEL_BRICK_IMPL_ARRAY must be defined
#endif //  VOXEL_BRICK_IMPL_ARRAY

float sdSphere(vec3 p, float d)
{
    return length(p) - d;
}

float sdBox(vec3 p, vec3 b)
{
    vec3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

bool getVoxel(uint offset, ivec3 c)
{
    // vec3  p = vec3(c) + vec3(0.5);
    // float d =
    //     min(max(-sdSphere(p, 7.5), sdBox(p, vec3(6.0))), -sdSphere(p, 25.0));
    // return d < 0.0;
    if (any(lessThan(c, ivec3(0))) || any(greaterThan(c, ivec3(7))))
    {
        return false;
    }

    return Voxel_isVisible(
        VOXEL_BRICK_IMPL_ARRAY[offset].voxels[c.x][c.y][c.z]);
}

IntersectionResult VoxelBrick_tryIntersect21(
    const uint offset, const vec3 cornerPos, const Ray ray)
{
    // routine to jump to the start of the cube
    Cube boundingCube;
    boundingCube.center      = cornerPos + VoxelBrick_EdgeLength / 2;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    IntersectionResult boundingCubeIntersection =
        Cube_tryIntersect(boundingCube, ray);

    if (!boundingCubeIntersection.intersection_occurred)
    {
        return IntersectionResult_getMiss();
    }
    // TODO: optimize to jump to start of traversal!

    const bool USE_BRANCHLESS_DDA = true;
    const int  MAX_RAY_STEPS      = 32;

    vec3 rayDir = ray.direction;
    vec3 rayPos = boundingCubeIntersection.maybe_hit_point - cornerPos;

    ivec3 mapPos = ivec3(floor(rayPos + 0.));

    vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);

    ivec3 rayStep = ivec3(sign(rayDir));

    vec3 sideDist =
        (sign(rayDir) * (vec3(mapPos) - rayPos) + (sign(rayDir) * 0.5) + 0.5)
        * deltaDist;

    bvec3 mask;

    for (int i = 0; i < MAX_RAY_STEPS; i++)
    {
        if (getVoxel(offset, mapPos))
        {
            Cube cube;
            cube.center = mapPos * Voxel_Size + cornerPos + Voxel_Size / 2.0;
            cube.edge_length = 1.0;

            IntersectionResult result = Cube_tryIntersect(cube, ray);

            result.maybe_color = Voxel_getLinearColor(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[mapPos.x % 8][mapPos.y % 8][mapPos.z % 8]);

            if (result.intersection_occurred)
            {
                return result;
            }
            else
            {
                return IntersectionResult_getError();
            }
        }

        mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
        sideDist += vec3(mask) * deltaDist;
        mapPos += ivec3(vec3(mask)) * rayStep;
    }

    return IntersectionResult_getMiss();
    // fragColor.rgb = vec3(mapPos) / 9.0;
    // fragColor.rgb = vec3(0.1 * noiseDeriv);
}

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