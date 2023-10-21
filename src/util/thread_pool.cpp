#include "thread_pool.hpp"

util::ThreadPool::ThreadPool(std::size_t numberOfWorkers)
    : sender {}
    , workers {numberOfWorkers}
{
    auto [localSender, receiver] = util::createChannel<std::function<void()>>();

    this->sender = std::move(localSender);

    for (std::thread& t : this->workers)
    {
        t = std::thread {[localReceiver = std::move(receiver)]
                         {
                             std::optional<std::function<void()>> maybeFunction;

                             while (true)
                             {
                                 auto receiveResult = localReceiver.receive();

                                 if (!receiveResult.has_value())
                                 {
                                     break;
                                 }

                                 std::invoke(*receiveResult);
                             }
                         }};
    }
}

util::ThreadPool::~ThreadPool()
{
    this->sender = Sender<std::function<void()>> {};

    for (std::thread& t : this->workers)
    {
        t.join();
    }
}