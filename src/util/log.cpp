#include "log.hpp"
#include "threads.hpp"
#include <atomic>
#include <fmt/chrono.h>
#include <fstream>
#include <latch>

namespace util
{

    const char* LoggingLevel_asString(LoggingLevel level) // NOLINT
    {
        switch (level)
        {
        case LoggingLevel::Trace:
            return "Trace";
        case LoggingLevel::Debug:
            return "Debug";
        case LoggingLevel::Log:
            return " Log ";
        case LoggingLevel::Warn:
            return "Warn ";
        case LoggingLevel::Fatal:
            return "Fatal";
        case LoggingLevel::Panic:
            return "Panic";
        default:
            return "Unknown level";
        };
    }

    class Logger
    {
    public:

        Logger();
        ~Logger();

        Logger(const Logger&)             = delete;
        Logger(Logger&&)                  = delete;
        Logger& operator= (const Logger&) = delete;
        Logger& operator= (Logger&&)      = delete;

        void send(std::string);
        void sendBlocking(std::string);

    private:
        std::unique_ptr<std::atomic<bool>> should_thread_close {};
        std::thread                        worker_thread;
        std::unique_ptr<moodycamel::ConcurrentQueue<std::string>> message_queue;
        std::unique_ptr<Mutex<std::ofstream>> log_file_handle;
    };

    Logger::Logger()
        : should_thread_close {std::make_unique<std::atomic<bool>>(false)}
        , message_queue {std::make_unique<
              moodycamel::ConcurrentQueue<std::string>>()}
        , log_file_handle {std::make_unique<util::Mutex<std::ofstream>>(
              std::ofstream {"verdigris_log.txt"})}
    {
        std::latch threadStartLatch {1};

        this->worker_thread = std::thread {
            [this, loggerConstructionLatch = &threadStartLatch]
            {
                std::ofstream logFileHandle {"verdigris_log.txt"};
                std::string   temporaryString {"INVALID MESSAGE"};

                std::atomic<bool>* shouldThreadStop {
                    this->should_thread_close.get()};

                moodycamel::ConcurrentQueue<std::string>* messageQueue {
                    this->message_queue.get()};

                loggerConstructionLatch->count_down();
                // after this latch:
                // `this` may be dangling
                // loggerConstructionLatch is dangling

                // Main loop
                while (!shouldThreadStop->load(std::memory_order_acquire))
                {
                    if (messageQueue->try_dequeue(temporaryString))
                    {
                        std::ignore = std::fwrite(
                            temporaryString.data(),
                            sizeof(char),
                            temporaryString.size(),
                            stdout);

                        this->log_file_handle->lock(
                            [&](std::ofstream& stream)
                            {
                                std::ignore = stream.write(
                                    temporaryString.c_str(),
                                    static_cast<std::streamsize>(
                                        temporaryString.size()));
                            });
                    }
                }

                // Cleanup loop
                while (messageQueue->try_dequeue(temporaryString))
                {
                    std::ignore = std::fwrite(
                        temporaryString.data(),
                        sizeof(char),
                        temporaryString.size(),
                        stdout);

                    this->log_file_handle->lock(
                        [&](std::ofstream& stream)
                        {
                            std::ignore = stream.write(
                                temporaryString.c_str(),
                                static_cast<std::streamsize>(
                                    temporaryString.size()));
                        });
                }
            }};

        threadStartLatch.wait();
    }
    Logger::~Logger()
    {
        if (this->should_thread_close != nullptr)
        {
            this->should_thread_close->store(true, std::memory_order_release);

            this->worker_thread.join();
        }
    }

    void Logger::send(std::string string)
    {
        if (!this->message_queue->enqueue(std::move(string)))
        {
            std::puts("Unable to send message to worker thread!\n");
            std::ignore =
                std::fwrite(string.data(), sizeof(char), string.size(), stderr);

            this->log_file_handle->lock(
                [&](std::ofstream& stream)
                {
                    std::ignore = stream.write(
                        string.c_str(),
                        static_cast<std::streamsize>(string.size()));
                });
        }
    }

    void Logger::sendBlocking(std::string string)
    {
        std::ignore =
            std::fwrite(string.data(), sizeof(char), string.size(), stderr);

        this->log_file_handle->lock(
            [&](std::ofstream& stream)
            {
                std::ignore = stream.write(
                    string.c_str(),
                    static_cast<std::streamsize>(string.size()));
            });
    }

    namespace
    {
        std::atomic<Logger*>      LOGGER {nullptr};                    // NOLINT
        std::atomic<LoggingLevel> LOGGING_LEVEL {LoggingLevel::Trace}; // NOLINT
    } // namespace

    void installGlobalLoggerRacy()
    {
        LOGGER.store(
            new Logger {}, std::memory_order_seq_cst); // NOLINT: Wwning pointer

        // this thread fence means that reading from LOGGER, even when
        // using Relaxed is guaranteed to acquire this new value. This is a non
        // trivial optimization and led to about a 10% performance uplift
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    void removeGlobalLoggerRacy()
    {
        Logger* const currentLogger =
            LOGGER.exchange(nullptr, std::memory_order_seq_cst);

        // doing very similar things with this fence here, we want relaxed
        // reading, but still to actually flush the change
        std::atomic_thread_fence(std::memory_order_seq_cst);

        assert(currentLogger != nullptr && "Logger was already nullptr!");

        delete currentLogger; // NOLINT: Owning pointer
    }

    void setLoggingLevel(LoggingLevel level)
    {
        LOGGING_LEVEL.store(level, std::memory_order_seq_cst);

        // Used for similar reasons as above, we want relaxed loading
        std::atomic_thread_fence(std::memory_order_seq_cst);

        logLog("Set logging level to {}", LoggingLevel_asString(level));
    }

    LoggingLevel getCurrentLevel()
    {
        return LOGGING_LEVEL.load(std::memory_order_relaxed);
    }

    void asynchronouslyLog(
        std::string                           message,
        LoggingLevel                          level,
        std::source_location                  location,
        std::chrono::system_clock::time_point time)
    {
        std::string output = fmt::format(
            "[{0}] [{1}] [{2}] {3}\n",
            [&] // 0 time
            {
                std::string workingString {31, ' '}; // NOLINT

                workingString = fmt::format(
                    "{:0%b %m/%d/%Y %I:%M}:{:%S}",
                    fmt::localtime(std::chrono::system_clock::to_time_t(time)),
                    time);

                workingString.erase(30, std::string::npos); // NOLINT

                workingString.at(workingString.size() - 7) = ':'; // NOLINT
                workingString.insert(workingString.size() - 3, ":");

                return workingString;
            }(),
            [&] // 1 location
            {
                constexpr std::array<std::string_view, 2> FolderIdentifiers {
                    "/src/", "/inc/"};
                std::string rawFileName {location.file_name()};

                std::size_t index = std::string::npos;

                for (std::string_view str : FolderIdentifiers)
                {
                    if (index != std::string::npos)
                    {
                        break;
                    }

                    index = rawFileName.find(str);
                }

                return fmt::format(
                    "{}:{}:{}",
                    rawFileName.substr(index + 1),
                    location.line(),
                    location.column());
            }(),
            [&] // 2 message level
            {
                return LoggingLevel_asString(level);
            }(),
            message);

        if (level >= LoggingLevel::Warn)
        {
            LOGGER.load(std::memory_order_relaxed)
                ->sendBlocking(std::move(output));
        }
        else
        {
            // the previous memory fences mean this is fine.
            LOGGER.load(std::memory_order_relaxed)->send(std::move(output));
        }
    }
} // namespace util
