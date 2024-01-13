#ifndef SRC_UTIL_ID_HPP
#define SRC_UTIL_ID_HPP

#include <string>

namespace util
{
    class UUID
    {
    public:
        UUID();
        ~UUID() = default;

        UUID(const UUID&)             = default;
        UUID(UUID&&)                  = default;
        UUID& operator= (const UUID&) = default;
        UUID& operator= (UUID&&)      = default;

        [[nodiscard]] bool     operator== (const UUID&) const = default;
        [[nodiscard]] bool     operator!= (const UUID&) const = default;
        [[nodiscard]] explicit operator std::string () const;
        [[nodiscard]] std::strong_ordering operator<=> (const UUID&) const;

        friend std::hash<UUID>;

    private:
        std::uint64_t timestamp;
        std::uint64_t id_number;
    }; // class UUID

} // namespace util

namespace std
{
    template<>
    struct hash<util::UUID>
    {
        std::size_t operator() (const util::UUID& uuid) const noexcept
        {
            std::size_t              seed {0};
            std::hash<std::uint64_t> hasher;

            auto hashCombine = [](std::size_t& seed_, std::size_t hash_)
            {
                hash_ += 0x9e3779b9 + (seed_ << 6) + (seed_ >> 2); // NOLINT
                seed_ ^= hash_;
            };

            hashCombine(seed, hasher(uuid.timestamp));
            hashCombine(seed, hasher(uuid.id_number));

            return seed;
        }
    };

} // namespace std

namespace util
{
    inline std::size_t hash_value(const util::UUID& uuid)
    {
        return std::hash<util::UUID> {}(uuid);
    }
} // namespace util

#endif // SEBUID_HPP
