#ifndef SRC_UTIL_MISC_HPP
#define SRC_UTIL_MISC_HPP

#include <concepts>

namespace util
{
    [[noreturn]] void debugBreak();

    template<class T>
    class AtomicUniquePtr
    {
    public:
        explicit AtomicUniquePtr() noexcept
            : owned_t {nullptr}
        {}

        template<class... Args>
        explicit AtomicUniquePtr(Args&&... args)
            requires std::is_constructible_v<T, Args...>
            : owned_t {new T {std::forward<Args>(args)...}}
        {}

        explicit AtomicUniquePtr(T&& t) // NOLINT
            requires std::is_move_constructible_v<T>
            : owned_t {new T {std::move<T>(t)}}
        {}

        ~AtomicUniquePtr()
        {
            delete this->owned_t;

            this->owned_t = nullptr;
        }

        AtomicUniquePtr(const AtomicUniquePtr&) = delete;
        AtomicUniquePtr(AtomicUniquePtr&& other) noexcept
            : owned_t {other.t}
        {
            other.owned_t = nullptr;
        }
        AtomicUniquePtr& operator= (const AtomicUniquePtr&) = delete;
        AtomicUniquePtr& operator= (AtomicUniquePtr&& other) noexcept
        {
            delete this->owned_t;

            this->owned_t = other.owned_t;

            other.owned_t = nullptr;
        }

        [[nodiscard]] T* leak()
        {
            T* output = this->owned_t;

            this->owned_t = nullptr;

            return output;
        }

        void destroy()
        {
            delete this->owned_t;
        }

    private:
        T* owned_t;
    };
} // namespace util

#endif // SRC_UTIL_MISC_HPP
