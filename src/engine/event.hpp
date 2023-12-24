#ifndef SRC_ENGINE_EVENT_DISPATCHER_HPP
#define SRC_ENGINE_EVENT_DISPATCHER_HPP

#include <concepts>
#include <expected>
#include <functional>
#include <future>
#include <memory>
#include <util/log.hpp>
#include <util/registrar.hpp>
#include <util/uuid.hpp>

namespace engine
{
    template<class... Ts>
        requires (std::is_copy_constructible_v<Ts...> || sizeof...(Ts) == 0)
    class Event
    {
    public:
        enum class EventProcessResult
        {
            Success       = 0,
            InvalidObject = 1,
        };
    public:
        /// Racy between calls. The first call to invoke must have finished for
        /// the next one to be called
        void invoke(Ts&&... args)
        {
            // Collect new callbacks
            {
                std::move_only_function<EventProcessResult(Ts...)>
                    maybeFunction {};

                while (this->subscriber_queue.try_dequeue(maybeFunction))
                {
                    this->registered_callbacks.insert(
                        std::pair {util::UUID {}, std::move(maybeFunction)});

                    maybeFunction = {};
                }
            }

            // Dispatch Callbacks
            {
                std::vector<std::future<std::expected<void, util::UUID>>>
                    calledEvents {};

                for (auto& [uuid, func] : this->registered_callbacks)
                {
                    calledEvents.push_back(std::async(
                        std::launch::async,
                        [... lambdaArgs = args,
                         &lambdaFunc    = func,
                         lambdaUUID = uuid] -> std::expected<void, util::UUID>
                        {
                            switch (lambdaFunc(lambdaArgs...))
                            {
                            case EventProcessResult::Success:
                                return {};
                            case EventProcessResult::InvalidObject:
                                return std::unexpected(lambdaUUID);
                            }
                        }));
                }

                for (std::future<std::expected<void, util::UUID>>& f :
                     calledEvents)
                {
                    std::expected<void, util::UUID> result = f.get();

                    if (!result.has_value())
                    {
                        this->registered_callbacks.erase(result.error());
                    }
                }
            }
        }

        template<class C>
        void subscribe(
            std::weak_ptr<C> weakInstance,
            void             (C::*memberFunction)(Ts...)) const
        {
            std::move_only_function<EventProcessResult(Ts...)> eventCallback {
                [=](Ts... args) -> EventProcessResult
                {
                    if (std::shared_ptr<C> instance = weakInstance.lock())
                    {
                        // fun fact shared_ptr doesn't overload operator ->*
                        ((*instance).*memberFunction)(args...);

                        return EventProcessResult::Success;
                    }
                    else
                    {
                        return EventProcessResult::InvalidObject;
                    }
                }};

            if (!this->subscriber_queue.try_enqueue(std::move(eventCallback)))
            {
                throw std::bad_alloc {};
            }
        }

    private:
        mutable moodycamel::ConcurrentQueue<
            std::move_only_function<EventProcessResult(Ts...)>>
            subscriber_queue;
        std::unordered_map<
            util::UUID,
            std::move_only_function<EventProcessResult(Ts...)>>
            registered_callbacks;
    };
} // namespace engine

#endif // SRC_ENGINE_EVENT_DISPATCHER_HPP
