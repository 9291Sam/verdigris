#ifndef SRC_UTIL_MISC_HPP
#define SRC_UTIL_MISC_HPP

#include <mutex>

namespace util
{
    [[noreturn]] void debugBreak();

    template<class... T>
    class Mutex
    {
    public:

        explicit Mutex(T... t)
            : tuple {std::forward<T>(t)...}
        {}
        ~Mutex() = default;

        Mutex(const Mutex&)                 = delete;
        Mutex(Mutex&&) noexcept             = default;
        Mutex& operator= (const Mutex&)     = delete;
        Mutex& operator= (Mutex&&) noexcept = default;

        void lock(std::invocable<T&...> auto func) noexcept(
            noexcept(std::apply(func, this->tuple)))
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

        bool try_lock(std::invocable<T&...> auto func) noexcept(
            noexcept(std::apply(func, this->tuple)))
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
} // namespace util

#endif // SRC_UTIL_MISC_HPP
