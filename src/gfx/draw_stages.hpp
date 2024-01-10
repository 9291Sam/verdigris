#ifndef SRC_GFX_DRAW_STAGES_HPP
#define SRC_GFX_DRAW_STAGES_HPP

namespace gfx
{
    // TODO: maybe not violate open-closed principle and make a proper
    // render graph, this is going to take a while and I don't care right
    // now!
    // circular dependencies my beloved
    enum class DrawStage
    {
        TopOfPipeCompute     = 0,
        VoxelDiscoveryPass   = 1,
        PostDiscoveryCompute = 2,
        DisplayPass          = 3,
        PostPipeCompute      = 4
    };
} // namespace gfx

#endif // SRC_GFX_DRAW_STAGES_HPP
