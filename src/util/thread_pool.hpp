#ifndef SRC_UTIL_THREAD_POOL_HPP
#define SRC_UTIL_THREAD_POOL_HPP

#include <atomic>
#include <barrier>
#include <blockingconcurrentqueue.h>
#include <functional>
#include <latch>
#include <optional>
#include <semaphore>
#include <thread>

namespace util
{
    template<class T>
    class FutureImpl
    {
    public:

        FutureImpl()
            : result_ready {1}
            , result {std::nullopt}
        {}
        ~FutureImpl() = default;

        FutureImpl(const FutureImpl&)             = delete;
        FutureImpl(FutureImpl&&)                  = delete;
        FutureImpl& operator= (const FutureImpl&) = delete;
        FutureImpl& operator= (FutureImpl&&)      = delete;

        void fulfill(T&& t)
        {
            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                assert(
                    !this->result.has_value()
                    && "fulfill called on a moved from future!");
            }

            this->result = std::forward<T>(t);

            this->result_ready.count_down();
        }

        T await()
        {
            this->result_ready.wait();

            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                assert(
                    this->result.has_value()
                    && "Await called on a moved from future!");
            }

            return std::move(*this->result);
        }

        std::optional<T> tryAwait()
        {
            if (this->result_ready.try_wait())
            {
                if constexpr (VERDIGRIS_ENABLE_VALIDATION)
                {
                    assert(
                        this->result.has_value()
                        && "Await called on a moved from future!");
                }

                return std::move(*result);
            }
            else
            {
                return std::nullopt;
            }
        }

    private:
        std::latch       result_ready;
        std::optional<T> result;
    };

    template<>
    class FutureImpl<void>
    {
    public:

        FutureImpl()
            : result_ready {1}
        {}
        ~FutureImpl() = default;

        FutureImpl(const FutureImpl&)             = delete;
        FutureImpl(FutureImpl&&)                  = delete;
        FutureImpl& operator= (const FutureImpl&) = delete;
        FutureImpl& operator= (FutureImpl&&)      = delete;

        void fulfill()
        {
            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                assert(
                    !this->result_ready.try_wait()
                    && "fulfill called on a moved from future!");
            }

            this->result_ready.count_down();
        }

        void await()
        {
            this->result_ready.wait();
        }

        bool tryAwait()
        {
            return this->result_ready.try_wait();
        }

    private:
        std::latch result_ready;
    };

    template<class T>
    class Future
    {
    public:

        Future()
            : impl {nullptr}
        {}
        ~Future()
        {
            if (this->impl != nullptr)
            {
                std::ignore = this->impl->await();
            }
        }

        Future(const Future&)             = delete;
        Future(Future&&)                  = default;
        Future& operator= (const Future&) = delete;
        Future& operator= (Future&&)      = default;

        decltype(auto) await()
        {
            return this->impl->await();
        }
        decltype(auto) tryAwait()
        {
            return this->impl->tryAwait();
        }

    private:
        friend class ThreadPool;

        Future(std::shared_ptr<FutureImpl<T>>);

        std::shared_ptr<FutureImpl<T>> impl;
    };

    // TODO: remove polling and use jthread and stop token
    class ThreadPool
    {
    public:

        ThreadPool(std::size_t numberOfThreads);
        ~ThreadPool();
        // TODO: deal with the case of a threadpool being
        // destructed while a barrier is still being worked on

        /// This function
        /// 1. takes each individual thread on the pool and blocks it from doing
        /// any more work until all threads have stopped
        /// 2. returns a semaphore that indicates when all the threads have
        /// stopped
        ///
        /// This is only when you need to synchronize all functions per
        std::binary_semaphore emitThreadBarrier();

        template<class... Ts, class R>
        void run(std::move_only_function<R(Ts...)> func, Ts&&... args) const
        {
            std::shared_ptr<FutureImpl<R>> futureImpl =
                std::make_shared<FutureImpl<R>>();

            std::move_only_function<void()> queueFunction =
                [lambdaImpl     = futureImpl,
                 lambdaFunc     = std::move(func),
                 ... lambdaArgs = std::forward<Ts>(args)]()
            {
                if constexpr (!std::same_as<R, void>)
                {
                    lambdaImpl->fulfill(std::invoke(lambdaFunc, lambdaArgs...));
                }
                else
                {
                    std::invoke(lambdaFunc, lambdaArgs...);

                    lambdaImpl->fulfill();
                }
            };

            if (!this->message_queue.enqueue(std::move(queueFunction)))
            {
                std::fputs(
                    "Failed to upload function to ThreadPool::message_queue!",
                    stderr);

                queueFunction();

                throw std::bad_alloc {};
            }
        };

    private:
        std::unique_ptr<std::atomic<std::barrier<void()>>> thread_barrier;
        std::atomic<std::shared_ptr<std::binary_semaphore>>
            thread_barrier_semaphore;

        mutable moodycamel::BlockingConcurrentQueue<
            std::move_only_function<void()>>
            message_queue;
    };
} // namespace util

#endif // SRC_UTIL_THREAD_POOL_HPP