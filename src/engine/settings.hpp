#ifndef SRC_ENGINE_SETTINGS_HPP
#define SRC_ENGINE_SETTINGS_HPP

#include <util/log.hpp>

namespace engine
{
    enum class Setting
    {
        EnableGFXValidation,
        EnableAppValidation,
    };

    class SettingsManager;

    const SettingsManager& getSettings();

    class SettingsManager
    {
    public:

        SettingsManager()  = default;
        ~SettingsManager() = default;

        SettingsManager(const SettingsManager&)             = delete;
        SettingsManager(SettingsManager&&)                  = delete;
        SettingsManager& operator= (const SettingsManager&) = delete;
        SettingsManager& operator= (SettingsManager&&)      = delete;

        template<Setting Setting>
        decltype(auto) lookupSetting() const noexcept
        {
            if constexpr (Setting == Setting::EnableGFXValidation)
            {
                return this->enable_gfx_validation.load(
                    std::memory_order_relaxed);
            }
            else if constexpr (Setting == Setting::EnableAppValidation)
            {
                return this->enable_app_validation.load(
                    std::memory_order_relaxed);
            }
            else
            {
                static_assert(false, "Setting not configured");
            }
        }

        void setSetting(Setting s, auto&& newValue) const
        {
            switch (s)
            {
            case Setting::EnableGFXValidation:
                util::assertFatal(
                    !gfx_validation_set, "gfx validation set multiple times");
                this->gfx_validation_set = true;

                this->enable_gfx_validation.store(
                    std::forward<decltype(newValue)>(newValue),
                    std::memory_order_seq_cst);

                std::atomic_thread_fence(std::memory_order_seq_cst);
                return;

            case Setting::EnableAppValidation:
                util::assertFatal(
                    !app_validation_set, "gfx validation set multiple times");
                this->app_validation_set = true;

                this->enable_app_validation.store(
                    std::forward<decltype(newValue)>(newValue),
                    std::memory_order_seq_cst);

                std::atomic_thread_fence(std::memory_order_seq_cst);
                return;
                // default:
                //     util::panic(
                //         "Tried to set invalid setting {} | {}",
                //         std::to_underlying(s),
                //         newValue);
            }
        }

    private:
        mutable std::atomic<bool> gfx_validation_set {false};
        mutable std::atomic<bool> app_validation_set {false};

#ifdef __cpp_lib_hardware_interference_size
        static constexpr std::size_t Alignment =
            std::hardware_destructive_interference_size;
#else
        // Good enough
        static constexpr std::size_t Alignment = 128;
#endif

        alignas(Alignment) mutable std::atomic<bool> enable_gfx_validation;
        alignas(Alignment) mutable std::atomic<bool> enable_app_validation;
    };

} // namespace engine

#endif // SRC_ENGINE_SETTINGS_HPP
