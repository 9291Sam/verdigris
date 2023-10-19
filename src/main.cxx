
import gfx.renderer;
import util.log;
import util.misc;
import stdlib;

int main()
{
    util::installGlobalLoggerRacy(std::make_unique<util::Logger>());

    std::atomic_thread_fence(std::memory_order_seq_cst);

    try
    {
        gfx::foo();

        util::logLog("Foo, {}", "bar");

        throw std::runtime_error {"asddd"};
    }
    catch (const std::exception& e)
    {
        util::logTrace("Verdigris crash | {}", e.what());
    }

    std::atomic_thread_fence(std::memory_order_seq_cst);

    std::ignore = util::removeGlobalLoggerRacy();
}