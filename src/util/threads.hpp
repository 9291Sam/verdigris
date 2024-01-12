#ifndef SRC_UTIL_THREADS_HPP
#define SRC_UTIL_THREADS_HPP

#include <future>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <vector>

namespace util
{

    template<class... T>
    class Mutex
    {
    public:

        explicit Mutex() = default;
        explicit Mutex(T&&... t) // NOLINT
            : tuple {std::forward<T>(t)...}
        {}
        ~Mutex() = default;

        Mutex(const Mutex&)                 = delete;
        Mutex(Mutex&&) noexcept             = default;
        Mutex& operator= (const Mutex&)     = delete;
        Mutex& operator= (Mutex&&) noexcept = default;

        decltype(auto) lock(std::invocable<T&...> auto func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock lock {this->mutex};

            return std::apply(func, this->tuple);
        }

        auto tryLock(std::invocable<T&...> auto&& func) const
            noexcept(noexcept(std::apply(func, this->tuple))) -> std::optional<
                std::decay_t<std::invoke_result_t<decltype(func), T&...>>>
            requires (!std::same_as<
                      void,
                      std::invoke_result_t<decltype(func), T&...>>)
        {
            std::unique_lock<std::mutex> lock {this->mutex, std::defer_lock};

            if (lock.try_lock())
            {
                return std::optional {std::apply(
                    std::forward<decltype(func)>(func), this->tuple)};
            }
            else
            {
                return std::nullopt;
            }
        }

        bool tryLock(std::invocable<T&...> auto&& func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
            requires std::
                same_as<void, std::invoke_result_t<decltype(func), T&...>>
        {
            std::unique_lock<std::mutex> lock {this->mutex, std::defer_lock};

            if (lock.try_lock())
            {
                std::apply(std::forward<decltype(func)>(func), this->tuple);
                return true;
            }
            else
            {
                return false;
            }
        }

        std::tuple_element_t<0, std::tuple<T...>> copyInner() const
            requires (sizeof...(T) == 1)
        {
            using V = std::tuple_element_t<0, std::tuple<T...>>;

            V output {};

            this->lock(
                [&](const V& data)
                {
                    output = data;
                });

            return output;
        }

    private:
        mutable std::mutex       mutex;
        mutable std::tuple<T...> tuple;
    }; // class Mutex

    template<class... T>
    class RwLock
    {
    public:

        explicit RwLock(T&&... t) // NOLINT
            : tuple {std::forward<T>(t)...}
        {}
        ~RwLock() = default;

        RwLock(const RwLock&)                 = delete;
        RwLock(RwLock&&) noexcept             = default;
        RwLock& operator= (const RwLock&)     = delete;
        RwLock& operator= (RwLock&&) noexcept = default;

        decltype(auto) writeLock(std::invocable<T&...> auto func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock lock {this->rwlock};

            return std::apply(func, this->tuple);
        }

        auto tryWriteLock(std::invocable<T&...> auto&& func) const
            noexcept(noexcept(std::apply(func, this->tuple))) -> std::optional<
                std::decay_t<std::invoke_result_t<decltype(func), T&...>>>
            requires (!std::same_as<
                      void,
                      std::invoke_result_t<decltype(func), T&...>>)
        {
            std::unique_lock lock {this->rwlock, std::defer_lock};

            if (lock.try_lock())
            {
                return std::optional {std::apply(
                    std::forward<decltype(func)>(func), this->tuple)};
            }
            else
            {
                return std::nullopt;
            }
        }

        decltype(auto) readLock(std::invocable<const T&...> auto func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::shared_lock lock {this->rwlock};

            return std::apply(func, this->tuple);
        }

        auto tryReadLock(std::invocable<T&...> auto&& func) const
            noexcept(noexcept(std::apply(func, this->tuple))) -> std::optional<
                std::decay_t<std::invoke_result_t<decltype(func), T&...>>>
            requires (!std::same_as<
                      void,
                      std::invoke_result_t<decltype(func), T&...>>)
        {
            std::shared_lock lock {this->rwlock, std::defer_lock};

            if (lock.try_lock())
            {
                return std::optional {std::apply(
                    std::forward<decltype(func)>(func), this->tuple)};
            }
            else
            {
                return std::nullopt;
            }
        }

        std::tuple_element_t<0, std::tuple<T...>> copyInner() const
            requires (sizeof...(T) == 1)
        {
            using V = std::tuple_element_t<0, std::tuple<T...>>;

            V output {};

            this->readLock(
                [&](const V& data)
                {
                    output = data;
                });

            return output;
        }

    private:
        mutable std::shared_mutex rwlock;
        mutable std::tuple<T...>  tuple;
    }; // class Mutex

    inline std::byte*
    threadedMemcpy(std::byte* dst, std::span<const std::byte> src)
    {
        const std::size_t numberOfThreads =
            std::thread::hardware_concurrency() * 3 / 4;
        const std::size_t threadDelegationSize =
            src.size_bytes() / numberOfThreads;

        std::vector<std::future<void>> futures {};
        futures.reserve(numberOfThreads);

        for (std::size_t i = 0; i < numberOfThreads; ++i)
        {
            std::size_t writeBlockStartOffset = threadDelegationSize * i;

            std::size_t writeBlockSize = threadDelegationSize;

            // if last then we need to add a little extra
            if (i != numberOfThreads - 1)
            {
                writeBlockSize += (src.size_bytes() % threadDelegationSize);
            }

            futures.push_back(std::async(
                std::launch::async,
                [=]
                {
                    std::memcpy(
                        dst + writeBlockStartOffset, // NOLINT
                        src.data() + writeBlockStartOffset,
                        writeBlockSize);
                }));
        }

        for (std::future<void>& f : futures)
        {
            f.get();
        }

        return dst;
    }

} // namespace util

#endif // SRC_UTIL_THREADS_HPP
