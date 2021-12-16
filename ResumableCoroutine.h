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
    struct ResumableCoroutine : public BaseCoroutine<T> {

        using BaseCoroutine<T>::BaseCoroutine;
        using HandleType = typename BaseCoroutine<T>::HandleType;

        T get() {
            DebugOutput("ResumableCoroutine::get()\n");

            if (!this->m_coroutineHandle.done()) {
                DebugOutput("ResumableCoroutine::get()::resuming coroutine\n");
                this->m_coroutineHandle.resume();
            } else {
                DebugOutput("ResumableCoroutine::get()::coroutine already done\n");
            }

            return BaseCoroutine<T>::get();
        }

        bool await_ready() {
            const auto ready = this->m_coroutineHandle.done();
#ifdef _DEBUG
            std::stringstream ss;
            ss << "ResumableCoroutine::await_ready(): " << (ready ? "is ready" : "isn't ready") <<std::endl;
#endif
            DebugOutput(ss.str().c_str());

            return this->m_coroutineHandle.done();
        }

        void await_suspend(std::experimental::coroutine_handle<> awaiting) {
            DebugOutput("ResumableCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaitable\n");
            this->m_coroutineHandle.resume();
            DebugOutput("ResumableCoroutine::await_suspend(std::experimental::coroutine_handle<> awaiting):: About to resume the awaiter\n");
            awaiting.resume();
        }

        auto await_resume() {
//            const auto ready = this->m_coroutineHandle.done();
//#ifdef _DEBUG
//            std::stringstream ss;
//            ss << "ResumableCoroutine::await_resume(): " << (ready ? "is ready" : "isn't ready") << std::endl;
//#endif
//            DebugOutput(ss.str().c_str());
//
//            return this->m_coroutineHandle.done();

            if (!this->m_coroutineHandle.done()) {
                this->m_coroutineHandle.resume();
            }

            const auto retval = BaseCoroutine<T>::get();

//#ifdef _DEBUG
//            std::stringstream ss;
//            ss << "ResumableCoroutine::await_resume(): retval = " << retval << std::endl;
//            DebugOutput(ss.str().c_str());
//#endif

            DebugOutput("ResumableCoroutine::await_resume()\n");

            return retval;
        }

        void resume() {
            this->m_coroutineHandle.resume();
        }

        bool done() {
            return this->m_coroutineHandle.done();
        }

        struct promise_type : public BaseCoroutine<T>::promise_type {

            auto get_return_object() {
                DebugOutput("ResumableCoroutine::promise_type::get_return_object()\n");
                return ResumableCoroutine<T>{HandleType::from_promise(*this)};
            }

            auto initial_suspend() {
                DebugOutput("ResumableCoroutine::promise_type::initial_suspend()\n");
                return std::experimental::suspend_always{};
            }

        };

    };

}
