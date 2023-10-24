#ifndef SRC_UTIL_THREADS_HPP
#define SRC_UTIL_THREADS_HPP

#include <future>
#include <mutex>
#include <span>
#include <vector>

namespace util
{

    template<class... T>
    class Mutex
    {
    public:

        explicit Mutex(T&&... t) // NOLINT
            : tuple {std::forward<T>(t)...}
        {}
        ~Mutex() = default;

        Mutex(const Mutex&)                 = delete;
        Mutex(Mutex&&) noexcept             = default;
        Mutex& operator= (const Mutex&)     = delete;
        Mutex& operator= (Mutex&&) noexcept = default;

        void lock(std::invocable<T&...> auto func) noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock lock {this->mutex};

            std::apply(func, this->tuple);
        }

        void lock(std::invocable<const T&...> auto func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock lock {this->mutex};

            std::apply(func, this->tuple);
        }

        bool
        try_lock(std::invocable<T&...> auto func) noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock<std::mutex> lock {this->mutex, std::defer_lock};

            if (lock.try_lock())
            {
                std::apply(func, this->tuple);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool try_lock(std::invocable<const T&...> auto func) const
            noexcept(noexcept(std::apply(func, this->tuple)))
        {
            std::unique_lock<std::mutex> lock {this->mutex, std::defer_lock};

            if (lock.try_lock())
            {
                std::apply(func, this->tuple);
                return true;
            }
            else
            {
                return false;
            }
        }

        std::tuple_element_t<0, std::tuple<T...>> copy_inner() const
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
        mutable std::mutex mutex;
        std::tuple<T...>   tuple;
    }; // class Mutex

    inline std::byte* threadedMemcpy(std::byte* dst, std::span<const std::byte> src)
    {
        const std::size_t numberOfThreads      = std::thread::hardware_concurrency() * 3 / 4;
        const std::size_t threadDelegationSize = src.size_bytes() / numberOfThreads;

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

    inline std::byte* threadedMemcpy(std::byte* dst, const std::byte* src, std::size_t size)
    {
        return threadedMemcpy(dst, std::span<const std::byte> {src, size});
    }

} // namespace util

#endif // SRC_UTIL_THREADS_HPP