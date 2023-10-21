#include "log.hpp"
#include <atomic>
#include <fmt/chrono.h>
#include <latch>

namespace util
{

    namespace
    {
        const char* LoggingLevel_asString(LoggingLevel level)
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
            default:
                return "Unknown level";
            };
        }
    } // namespace

    class Logger
    {
    public:

        Logger();
        ~Logger();

        Logger(const Logger&)             = delete;
        Logger(Logger&&)                  = delete;
        Logger& operator= (const Logger&) = delete;
        Logger& operator= (Logger&&)      = delete;

        void send(std::string string);

    private:
        std::unique_ptr<std::atomic<bool>> should_thread_close {};
        std::thread                        worker_thread;
        std::unique_ptr<moodycamel::ConcurrentQueue<std::string>> message_queue;
    };

    Logger::Logger()
        : should_thread_close {std::make_unique<std::atomic<bool>>(false)}
        , message_queue {
              std::make_unique<moodycamel::ConcurrentQueue<std::string>>()}
    {
        std::latch threadStartLatch {1};

        this->worker_thread = std::thread {
            [this, loggerConstructionLatch = &threadStartLatch]
            {
                std::string temporary_string {"INVALID MESSAGE"};

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
                    if (messageQueue->try_dequeue(temporary_string))
                    {
                        std::fwrite(
                            temporary_string.data(),
                            sizeof(char),
                            temporary_string.size(),
                            stdout);
                    }
                }

                // Cleanup loop
                while (messageQueue->try_dequeue(temporary_string))
                {
                    std::fwrite(
                        temporary_string.data(),
                        sizeof(char),
                        temporary_string.size(),
                        stdout);
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
        if (!this->message_queue->enqueue(string))
        {
            std::puts("Unable to send message to worker thread!\n");
            std::fwrite(string.data(), sizeof(char), string.size(), stdout);
        }
    }

    namespace
    {
        static std::atomic<Logger*>      GLOBAL_LOGGER {nullptr};
        static std::atomic<LoggingLevel> GLOBAL_LOGGING_LEVEL {
            LoggingLevel::Log};
    } // namespace

    void installGlobalLoggerRacy()
    {
        GLOBAL_LOGGER.store(new Logger {}, std::memory_order_seq_cst);

        // this thread fence means that reading from GLOBAL_LOGGER, even when
        // using Relaxed is guaranteed to acquire this new value. This is a non
        // trivial optimization and led to about a 10% performance uplift
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    void removeGlobalLoggerRacy()
    {
        Logger* const currentLogger =
            GLOBAL_LOGGER.exchange(nullptr, std::memory_order_seq_cst);

        // doing very similar things with this fence here, we want relaxed
        // reading, but still to actually flush the change
        std::atomic_thread_fence(std::memory_order_seq_cst);

        assert(currentLogger != nullptr && "Logger was already nullptr!");

        delete currentLogger;
    }

    void setLoggingLevel(LoggingLevel level)
    {
        GLOBAL_LOGGING_LEVEL.store(level, std::memory_order_seq_cst);

        // Used for similar reasons as above, we want relaxed loading
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    LoggingLevel getCurrentLevel()
    {
        return GLOBAL_LOGGING_LEVEL.load(std::memory_order_relaxed);
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
                std::string workingString {31, ' '};

                workingString = fmt::format(
                    "{:0%b %m/%d/%Y %I:%M}:{:%S}",
                    fmt::localtime(std::time(nullptr)),
                    std::chrono::system_clock::now());

                workingString.erase(30, std::string::npos);

                workingString.at(workingString.size() - 7) = ':';
                workingString.insert(workingString.size() - 3, ":");

                return workingString;
            }(),
            [&] // 1 location
            {
                constexpr std::array<std::string_view, 2> FOLDER_IDENTIFIERS {
                    "/src/", "/inc/"};
                std::string raw_file_name {location.file_name()};

                std::size_t index = std::string::npos;

                for (std::string_view str : FOLDER_IDENTIFIERS)
                {
                    if (index != std::string::npos)
                    {
                        break;
                    }

                    index = raw_file_name.find(str);
                }

                return fmt::format(
                    "{}:{}", raw_file_name.substr(index + 1), location.line());
            }(),
            [&] // 2 message level
            {
                return LoggingLevel_asString(level);
            }(),
            message);

        // the previous memory fences mean this is fine.
        GLOBAL_LOGGER.load(std::memory_order_relaxed)->send(std::move(output));
    }
} // namespace util
