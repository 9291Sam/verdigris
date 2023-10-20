import gfx.renderer;
import util.log;
import util.misc;

#include <atomic>
#include <memory>

int main()
{
    util::installGlobalLoggerRacy(std::make_unique<util::Logger>());

    try
    {
        gfx::Renderer renderer {};

        renderer.drawFrame();
    }
    catch (const std::exception& e)
    {
        util::logTrace("Verdigris crash | {}", e.what());
    }

    std::ignore = util::removeGlobalLoggerRacy();
}