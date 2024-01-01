#include <engine/event.hpp>
#include <engine/settings.hpp>
#include <future>
#include <game/game.hpp>
#include <gfx/object.hpp>
#include <gfx/renderer.hpp>
#include <glm/common.hpp>
#include <magic_enum_all.hpp>
#include <util/block_allocator.hpp>
#include <util/log.hpp>
#include <util/noise.hpp>

void setDefaultSettings(engine::SettingsManager&);
void parseCommandLineArgumentsAndUpdateSettings(int argc, char** argv);

int main(int argc, char** argv)
{
    try
    {
        util::installGlobalLoggerRacy();
        parseCommandLineArgumentsAndUpdateSettings(argc, argv);

        gfx::Renderer renderer {};
        game::Game    game {renderer};

        std::atomic<bool> shouldStop {false};

        std::future<void> gameLoop = std::async(
            [&]
            {
                while (game.continueTicking()
                       && !shouldStop.load(std::memory_order_acquire))
                {
                    game.tick();
                }

                shouldStop.store(true, std::memory_order_release);
            });

        while (renderer.continueTicking()
               && !shouldStop.load(std::memory_order_acquire))
        {
            renderer.drawFrame();
        }

        shouldStop.store(true, std::memory_order_release);

        gameLoop.wait();
    }
    catch (const std::exception& e)
    {
        util::logFatal("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}

// this is bad... TODO: abstract this and make a proper argument parser, you're
// going to need it
void parseCommandLineArgumentsAndUpdateSettings(int argc, char** argv)
{
    bool customLoggingLevelSet = false;
    bool setGFXValidation      = false;
    bool setAppValidation      = false;

    for (int i = 0; i < argc; ++i)
    {
        if (std::strcmp("-v", argv[i]) == 0) // NOLINT
        {
            engine::getSettings().setSetting(
                engine::Setting::EnableGFXValidation, true);
            setGFXValidation = true;

            engine::getSettings().setSetting(
                engine::Setting::EnableAppValidation, true);
            setAppValidation = true;

            util::setLoggingLevel(util::LoggingLevel::Trace);
            customLoggingLevelSet = true;
        }
        else if (std::strcmp("--force-gfx-validation", argv[i]) == 0) // NOLINT
        {
            engine::getSettings().setSetting(
                engine::Setting::EnableGFXValidation, true);
            setGFXValidation = true;
        }
        else if (std::strcmp("--force-app-validation", argv[i]) == 0) // NOLINT
        {
            engine::getSettings().setSetting(
                engine::Setting::EnableAppValidation, true);
            setAppValidation = true;
        }
        else if (std::strcmp("--force-logging-level", argv[i]) == 0) // NOLINT
        {
            util::assertFatal(i + 1 < argc, "Not enough arguments");
            const char* maybeLoggingLevel = argv[i + 1]; // NOLINT

            magic_enum::enum_for_each<util::LoggingLevel>(
                [&](util::LoggingLevel l)
                {
                    const char* levelAsString = util::LoggingLevel_asString(l);

                    if (std::strcmp(levelAsString, maybeLoggingLevel) == 0)
                    {
                        util::setLoggingLevel(l);
                        customLoggingLevelSet = true;
                    }
                });

            util::assertFatal(
                customLoggingLevelSet,
                "Invalid logging level string {}",
                argv[i + 1]); // NOLINT

            ++i;
            // customLoggingLevelSet = true;
        }
        else
        {
            if (i == 0)
            {
                continue;
            }
            else
            {
                // NOLINTNEXTLINE
                util::logWarn("Unrecognized command line argument {}", argv[i]);
            }
        }
    }

    if (!customLoggingLevelSet)
    {
#ifndef NDEBUG
        util::setLoggingLevel(util::LoggingLevel::Trace);
#else
        util::setLoggingLevel(util::LoggingLevel::Log);
#endif
    }

    using engine::Setting;

    // set default settings otherwise
    magic_enum::enum_for_each<Setting>(
        [&](Setting s)
        {
            switch (s)
            {
            case Setting::EnableGFXValidation:
                if (!setGFXValidation)
                {
#ifndef NDEBUG
                    engine::getSettings().setSetting(
                        Setting::EnableGFXValidation, true);
#else
                    engine::getSettings().setSetting(
                        Setting::EnableGFXValidation, false);
#endif
                }
                break;
            case Setting::EnableAppValidation:
                if (!setAppValidation)
                {
#ifndef NDEBUG
                    engine::getSettings().setSetting(
                        Setting::EnableAppValidation, true);
#else
                    engine::getSettings().setSetting(
                        Setting::EnableAppValidation, false);
#endif
                }
                break;
            }
        });

    util::logLog(
        "Validations? | Gfx: {} | App: {}",
        engine::getSettings().lookupSetting<Setting::EnableGFXValidation>(),
        engine::getSettings().lookupSetting<Setting::EnableAppValidation>());
}

// void directIndirectAlgorithm()
// {

//     // Face refers to cubes + the voxel directely adjacent to them
//     struct
//     {
//         std::vector<Face> discoveredDirectFaces;
//         Image             directFaces;
//     } = rasterPass();

//     struct
//     {
//         std::map<Face, Face> discoveredIndirectFaces;
//     } = computePass();

//     // list the indirect faces that influence another face

//     std::vector<Face> directAndIndirectFaces {};

//     struct
//     {
//         std::map<Face, Color> faceColors
//     } = computePass(sum(directAndIndirectFaces))

//     // comebine faces as the result of direct and indirect colors
// }