#ifndef SRC_UTIL_THREAD_POOL_HPP
#define SRC_UTIL_THREAD_POOL_HPP

#include <atomic>
#include <barrier>
#include <blockingconcurrentqueue.h>
#include <expected>
#include <functional>
#include <future>
#include <latch>
#include <optional>
#include <semaphore>
#include <thread>

namespace util
{
    template<class T>
    class Sender;

    template<class T>
    class Receiver;

    template<class T>
    struct ChannelControlBlock
    {
        ChannelControlBlock() = default;

        ChannelControlBlock(const ChannelControlBlock&)             = delete;
        ChannelControlBlock(ChannelControlBlock&&)                  = delete;
        ChannelControlBlock& operator= (const ChannelControlBlock&) = delete;
        ChannelControlBlock& operator= (ChannelControlBlock&&)      = delete;

        moodycamel::BlockingConcurrentQueue<T> queue;
        std::size_t                            number_of_senders;
        std::size_t                            number_of_receivers;
    };

    // TODO: figure out friending templates from templates
    template<class T>
    [[nodiscard]] std::pair<Sender<T>, Receiver<T>> createChannel()
        requires std::is_move_constructible_v<T>
    {
        std::shared_ptr<ChannelControlBlock<T>> block =
            std::make_shared<ChannelControlBlock<T>>();

        block->number_of_senders   = 1;
        block->number_of_receivers = 1;

        return std::make_pair(Sender {block}, Receiver {block});
    }

    template<class T>
    class Sender
    {
    public:

        Sender()
            : control_block {nullptr}
        {}
        Sender(std::shared_ptr<ChannelControlBlock<T>> newBlock)
            : control_block {std::move(newBlock)}
        {}
        ~Sender()
        {
            if (this->control_block != nullptr)
            {
                this->control_block->number_of_senders -= 1;
            }
        }

        explicit Sender(const Sender& other)
            : control_block {other.control_block}
        {
            this->control_block->number_of_senders += 1;
        }
        Sender(Sender&&)                  = default;
        Sender& operator= (const Sender&) = delete;
        Sender& operator= (Sender&&)      = default;

        void send(T&& t) const
        {
            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                assert(this->control_block != nullptr);
                assert(this->control_block->number_of_receivers > 0);
            }

            if (!this->control_block->queue.enqueue(std::forward<T>(t)))
            {
                throw std::bad_alloc {};
            }
        }

    private:
        std::shared_ptr<ChannelControlBlock<T>> control_block;
    };

    template<class T>
    class Receiver
    {
    public:

        struct NoSendersAlive
        {};

    public:

        Receiver()
            : control_block {nullptr}
        {}
        Receiver(std::shared_ptr<ChannelControlBlock<T>> newBlock)
            : control_block {std::move(newBlock)}
        {}
        ~Receiver()
        {
            if (this->control_block != nullptr)
            {
                this->control_block->number_of_receivers -= 1;
            }
        }

        explicit Receiver(const Receiver& other)
            : control_block {other.control_block}
        {
            this->control_block->number_of_receivers += 1;
        }
        Receiver(Receiver&&)                  = default;
        Receiver& operator= (const Receiver&) = delete;
        Receiver& operator= (Receiver&&)      = default;

        [[nodiscard]] std::expected<std::optional<T>, NoSendersAlive>
        try_receive() const
        {
            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                assert(this->control_block != nullptr);
            }

            std::optional<T> maybeOutput = std::nullopt;

            this->control_block->queue.try_dequeue(maybeOutput);

            if (maybeOutput.has_value())
            {
                return maybeOutput;
            }
            else
            {
                if (this->control_block->number_of_senders == 0)
                {
                    return std::unexpected(NoSendersAlive {});
                }
                else
                {
                    return std::nullopt;
                }
            }
        }

        [[nodiscard]] std::expected<T, NoSendersAlive> receive() const
        {
            while (true)
            {
                if (this->control_block->number_of_senders == 0)
                {
                    return std::unexpected(NoSendersAlive {});
                }

                std::optional<T> maybeResult;

                this->control_block->queue.wait_dequeue_timed(maybeResult, 750);

                if (maybeResult.has_value())
                {
                    return *maybeResult;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }

    private:
        std::shared_ptr<ChannelControlBlock<T>> control_block;
    };
} // namespace util

#endif // SRC_UTIL_THREAD_POOL_HPP