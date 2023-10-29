#include "uuid.hpp"
#include "misc.hpp"
#include <atomic>
#include <chrono>
#include <fmt/format.h>
#include <random>

namespace util
{
    UUID::UUID()
    {
        static std::atomic<std::uint64_t> id {std::random_device {}()};

        this->timestamp = util::crc<std::uint64_t>(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count()));
        this->id_number = util::crc<std::uint64_t>(id++);
    }

    UUID::operator std::string () const
    {
        return fmt::format(
            "{:08X}~{:08X}~{:08X}~{:08X}",
            this->timestamp >> 32,          // NOLINT
            this->timestamp & 0xFFFF'FFFF,  // NOLINT
            this->id_number >> 32,          // NOLINT
            this->id_number & 0xFFFF'FFFF); // NOLINT
    }

    std::strong_ordering util::UUID::operator<=> (const UUID& other) const
    {
        return std::tie(this->timestamp, this->id_number)
           <=> std::tie(other.timestamp, other.id_number);
    }
} // namespace util
