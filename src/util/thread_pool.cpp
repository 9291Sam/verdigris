#include "thread_pool.hpp"

util::ThreadPool::ThreadPool(std::size_t numberOfWorkers)
    : thread_barrier {nullptr}
    , thread_barrier_semaphore {std::make_shared<std::binary_semaphore>(1)}
    , message_queue {}
    , workers {numberOfWorkers}
{
    // std::make_unique<
    //     std::unique_ptr<std::atomic<std::barrier<void() noexcept>>>>(nullptr)

    std::barrier<void()> barrier {16, [] noexcept {}};

    for (std::thread& t : this->workers)
    {
        t = std::thread {
            [pinnedQueue         = this->message_queue.get(),
             pinnedBarrier       = this->thread_barrier.get(),
             pinnedBarrierActive = this->barrier_active.get(),
             pinnedShouldStop    = this->should_stop.get()]
            {
                std::optional<std::move_only_function<void()>> maybeFunction;

                while (true)
                {
                    if (pinnedQueue->wait_dequeue_timed(maybeFunction, 750))
                    {
                        std::invoke(*maybeFunction);
                    }

                    if (pinnedShouldStop->load(std::memory_order_acquire))
                    {
                        break;
                    }

                    if (pinnedBarrierActive->load(std::memory_order_acquire))
                    {
                        // pinnedBarrier.?
                    }
                }
            }};
    }
}