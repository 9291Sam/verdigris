Voxel ray tracing

combined images

make Player blessed to continue moving when game ticking is stalled

make a buffer upload abstraction to test how much data can be uploaded per frame consistently

Add views of cpu and gpu memory usage in debug menu

Add toggles for validation layers

Show logged messages to stderr, and log them to a file

event system (set player position)
command line arguments test and loggigng

seperate raster and rt gpu threads?

seperate toggles for verdigris validation and vulkan validation

bad apple as a tech demo

proper argument parser (console commands and command line args)\

track where the player is moving and if theyre going to need to completely reallocate where the octree is centered do that 
at the same time this also allows for you to deal with a floating origin just ifne 

add etst in the raycasting shader to cehck if that pixel is the ceter of teh screen if so then you can put that in a buffer that the cpu reads tosee were the center of the world is 


add returning of values from util::Mutex lambdas


cam you suet eh dedicated transfer hardware on the gpu i.e a queue with only Transfer

to upload the brick informatio nso that you dotn have to store it on the cpu side at all? 

i.e you can return some type of astynchronous future request for it that gets populated at the end of each command buffer submission (follow the barrier) 

fix the cmake script lmfao