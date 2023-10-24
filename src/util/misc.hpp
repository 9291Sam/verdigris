#ifndef SRC_UTIL_MISC_HPP
#define SRC_UTIL_MISC_HPP

#include <atomic>
#include <concepts>
#include <type_traits>

namespace util
{
    [[noreturn]] void debugBreak();

    template<class T>
    concept Integer = requires {
        requires std::integral<T>;
        requires !std::floating_point<T>;
        requires !std::same_as<T, bool>;
        requires !std::is_pointer_v<T>;
    };

    template<class T>
    concept UnsignedInteger = requires {
        requires Integer<T>;
        requires std::is_unsigned_v<T>;
    };

    template<class T>
    concept Arithmetic = requires {
        requires std::integral<T> || std::floating_point<T>;
        requires !std::same_as<T, bool>;
        requires !std::is_pointer_v<T>;
    };

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
            delete this->owned_t.exchange(nullptr, std::memory_order_acq_rel); // NOLINT
        }

        AtomicUniquePtr(const AtomicUniquePtr&) = delete;
        AtomicUniquePtr(AtomicUniquePtr&& other) noexcept
            : owned_t {nullptr}
        {
            this->owned_t.store(
                other.owned_t.exchange(nullptr, std::memory_order_acq_rel),
                std::memory_order_acq_rel);
        }
        AtomicUniquePtr& operator= (const AtomicUniquePtr&) = delete;
        AtomicUniquePtr& operator= (AtomicUniquePtr&& other) noexcept
        {
            delete this->owned_t.exchange(nullptr, std::memory_order_acq_rel); // NOLINT

            this->owned_t = other.owned_t.exchange(nullptr, std::memory_order_acq_rel);
        }

        // Primary interaction method
        operator std::atomic<T*>& () const // NOLINT: implicit
        {
            return this->owned_t;
        }

        [[nodiscard]] T* leak() noexcept
        {
            return this->owned_t.exchange(nullptr, std::memory_order_acq_rel);
        }

        void destroy() noexcept
        {
            delete this->owned_t.exchange(nullptr, std::memory_order_acq_rel); // NOLINT
        }

    private:
        std::atomic<T*> owned_t;
    };
} // namespace util

#endif // SRC_UTIL_MISC_HPP
