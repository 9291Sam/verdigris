#ifndef SRC_UTIL_REGISTRAR_HPP
#define SRC_UTIL_REGISTRAR_HPP

#include "concurrentqueue.h"
#include "util/log.hpp"
#include <atomic>
#include <concepts>
#include <new>
#include <optional>
#include <type_traits>
#include <unordered_map>

namespace util
{
    // think a flushable map, thats only occassionally accessed, not useful for
    // consistent concurrent random modification
    // but useful for delta changes between frames.
    template<class Key, class Value>
        requires std::copyable<Key> && std::copyable<Value>
    class Registrar
    {
    public:

        Registrar()  = default;
        ~Registrar() = default;

        void insert(std::pair<Key, Value> set) const
        {
            if (!this->sets_to_add.enqueue(std::move(set)))
            {
                throw std::bad_alloc {};
            }
        }
        void remove(Key key) const
        {
            if (!this->keys_to_remove.enqueue(std::move(key)))
            {
                throw std::bad_alloc {};
            }
        }

        std::vector<std::pair<Key, Value>> access()
        {
            // Flush adds
            {
                std::optional<std::pair<Key, Value>> dequeueKeyValue;
                while (this->sets_to_add.try_dequeue(dequeueKeyValue))
                {
                    auto& [key, value] = *dequeueKeyValue;

                    if constexpr (VERDIGRIS_ENABLE_VALIDATION)
                    {
                        util::assertFatal(
                            !this->map.contains(key),
                            "Key was already in Registrar!");
                    }

                    this->map[std::move(key)] = std::move(value);

                    this->map_size.fetch_add(1, std::memory_order_acq_rel);

                    dequeueKeyValue = std::nullopt;
                }
            }

            // Flush removals
            {
                std::optional<Key> dequeueKey;
                while (this->keys_to_remove.try_dequeue(dequeueKey))
                {
                    if constexpr (VERDIGRIS_ENABLE_VALIDATION)
                    {
                        util::assertFatal(
                            this->map.contains(*dequeueKey),
                            "Key was not present in Registrar");
                    }

                    this->map.erase(*dequeueKey);

                    this->map_size.fetch_sub(1, std::memory_order_acq_rel);

                    dequeueKey = std::nullopt;
                }
            }

            // Generate output
            std::vector<std::pair<Key, Value>> output;
            output.reserve(this->map_size.load(std::memory_order_acquire));

            for (auto& keyValue : this->map)
            {
                output.push_back(keyValue);
            }

            return output;
        }
    private:

        mutable moodycamel::ConcurrentQueue<std::pair<Key, Value>> sets_to_add;
        mutable moodycamel::ConcurrentQueue<Key> keys_to_remove;

        std::atomic<std::size_t>       map_size = 0;
        std::unordered_map<Key, Value> map;
    };
} // namespace util

#endif // SRC_UTIL_REGISTRAR_HPP