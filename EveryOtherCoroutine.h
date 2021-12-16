#pragma once

#include "BaseCoroutine.h"

#include <sstream>

//#ifdef _DEBUG
//#ifndef OUTPUT_DEBUG_STRINGS
//#define DebugOutput(cstr)
//#endif
//#endif

namespace ndtech {

    template<typename T>
    struct EveryOtherCoroutine : public BaseCoroutine<T> {

        using BaseCoroutine<T>::BaseCoroutine;
        using HandleType = typename BaseCoroutine<T>::HandleType;

        bool suspend = true;

        T get() {
            DebugOutput("EveryOtherCoroutine::get()\n");

            if (!this->m_coroutineHandle.done()) {
                DebugOutput("EveryOtherCoroutine::get()::resuming coroutine\n");
                this->m_coroutineHandle.resume();
            } else {
                DebugOutput("EveryOtherCoroutine::get()::coroutine already done\n");
            }

            return BaseCoroutine<T>::get();
        }

        bool await_ready() {
            const auto ready = !suspend;
            suspend = ready;

#ifdef _DEBUG
            std::stringstream ss;
            ss << "EveryOtherCoroutine::await_ready(): " << (ready ? "is ready" : "isn't ready") << std::endl;
#endif
            DebugOutput(ss.str().c_str());

            return ready;
        }

        void await_suspend(std::experimental::coroutine_handle<> awaiting) {
            DebugOutput("EveryOtherCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaitable\n");
            this->m_coroutineHandle.resume();
            DebugOutput("EveryOtherCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaiter\n");
            awaiting.resume();
        }

        auto await_resume() {
            const auto returnValue = this->m_coroutineHandle.promise().getValue();
#ifdef _DEBUG
            std::stringstream ss;
            ss << "EveryOtherCoroutine::await_resume(): returning value = " << returnValue << std::endl;
#endif
            DebugOutput(ss.str().c_str());

            return returnValue;
        }

        struct promise_type : public BaseCoroutine<T>::promise_type {

            auto get_return_object() {
                DebugOutput("EveryOtherCoroutine::promise_type::get_return_object()\n");
                return EveryOtherCoroutine<T>{HandleType::from_promise(*this)};
            }

            auto initial_suspend() {
                DebugOutput("EveryOtherCoroutine::promise_type::initial_suspend()\n");
                return std::experimental::suspend_always{};
            }

        };

    };

}
