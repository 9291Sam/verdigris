# Verdigris
An experimental voxel renderer with the goal of creating endless fully editable landscapes. The goal is not to have realism, however there is a specific art style in mind

I have plans to turn this into an adventure game with an extremely specific art style, however that goal is still many, *many* years away at a minimum


## Demonstration
[![embed_chunking_test.mp4](embed_chunking_test.mp4)](embed_chunking_test.mp4)

- 25 512^3 voxel chunks being generated concurrently
- Compute shader raytracing a sphere at a position aligned with a rasterized object
- Pervasive use of fork-join parallelism
- Isolated render and tick loops
- Probably a lot more, feel free to ask me questions. Discord: `9291Sam`


## Build Instructions

1. Clone the repo
2. Download the vulkan sdk [![https://vulkan.lunarg.com/](https://vulkan.lunarg.com/)](https://vulkan.lunarg.com/)
3. Configure cmake (other dependencies should be downloaded and automatically built)
    - NOTE: this may take a while (5 minutes++ for first configure, ~10s after)
    - NOTE: ninja is the only tested generator
