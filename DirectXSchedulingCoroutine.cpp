#include "pch.h"

#include "DirectXSchedulingCoroutine.h"

#include <sstream>

//#ifdef _DEBUG
//#ifndef OUTPUT_DEBUG_STRINGS
//#define DebugOutput(cstr)
//#endif
//#endif

namespace ndtech {

    int DirectXSchedulingCoroutine::get() {
        DebugOutput("DirectXSchedulingCoroutine::get()\n");

        if (!this->m_coroutineHandle.done()) {
            DebugOutput("DirectXSchedulingCoroutine::get()::resuming coroutine\n");
            this->m_coroutineHandle.resume();
        }
        else {
            DebugOutput("DirectXSchedulingCoroutine::get()::coroutine already done\n");
        }

        return BaseCoroutine<int>::get();
    }

    bool DirectXSchedulingCoroutine::await_ready() {
        const auto ready = this->m_coroutineHandle.done();
#ifdef _DEBUG
        std::stringstream ss;
        ss << "DirectXSchedulingCoroutine::await_ready(): " << (ready ? "is ready" : "isn't ready") << std::endl;
#endif
        DebugOutput(ss.str().c_str());

        return this->m_coroutineHandle.done();
    }

    void DirectXSchedulingCoroutine::await_suspend(std::experimental::coroutine_handle<> awaiting) {
        DebugOutput("DirectXSchedulingCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaitable\n");
        this->m_coroutineHandle.resume();
        DebugOutput("DirectXSchedulingCoroutine::await_suspend(std::experimental::coroutine_handle<> awaiting):: About to resume the awaiter\n");
        //awaiting.resume();
    }

    auto DirectXSchedulingCoroutine::await_resume() {
        const auto returnValue = this->m_coroutineHandle.promise().getValue();
#ifdef _DEBUG
        std::stringstream ss;
        ss << "DirectXSchedulingCoroutine::await_resume(): returning value = " << returnValue << std::endl;
#endif
        DebugOutput(ss.str().c_str());

        //return returnValue;
        return this->m_coroutineHandle.done();
    }

    void DirectXSchedulingCoroutine::resume() {
        this->m_coroutineHandle.resume();
    }

    bool DirectXSchedulingCoroutine::done() {
        return this->m_coroutineHandle.done();
    }

    auto DirectXSchedulingCoroutine::promise_type::get_return_object() {
        DebugOutput("DirectXSchedulingCoroutine::promise_type::get_return_object()\n");
        return DirectXSchedulingCoroutine{HandleType::from_promise(*this)};
    }

    auto DirectXSchedulingCoroutine::promise_type::initial_suspend() {
        DebugOutput("DirectXSchedulingCoroutine::promise_type::initial_suspend()\n");
        return std::experimental::suspend_always{};
    }

}