#include "settings.hpp"

namespace engine
{
    namespace
    {
        const SettingsManager GlobalSettingsManager {};
    } // namespace

    const SettingsManager& getSettings()
    {
        return GlobalSettingsManager;
    }
} // namespace engine