# Verdigris
An experimental rasterizing and raytracing voxel renderer with the goal of creating endless, fully editable, semi-realistic natural scenes.


## Demonstration video
https://github.com/9291Sam/verdigris/assets/25729215/1b843f5d-4562-487f-9fb6-f400d29b9cc6


### Features
- 16 512^3 voxel chunks being generated concurrently
- Compute shader raytracing at aligned positions with rasterized objects
- Isolated render and tick threads
- Pervasive use of fork-join parallelism
- Probably a lot more, this has been my personal project for a bit. Feel free to ask me questions. Discord: `9291Sam`


### Build Instructions 
1. Clone the repo
2. Download and install the vulkan sdk at https://vulkan.lunarg.com
3. Configure cmake 
    - All dependencies will be populated and automatically built
    - This may take a while, up to 5 minutes for first configure, ~10s after
    - **Ninja is the only tested generator**
