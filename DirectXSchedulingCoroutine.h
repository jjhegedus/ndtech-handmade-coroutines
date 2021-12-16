#pragma once

#include "BaseCoroutine.h"

#include <sstream>

//#ifdef _DEBUG
//#ifndef OUTPUT_DEBUG_STRINGS
//#define DebugOutput(cstr)
//#endif
//#endif

namespace ndtech {

    struct DirectXSchedulingCoroutine : public BaseCoroutine<int> {

        using BaseCoroutine<int>::BaseCoroutine;
        using HandleType = typename BaseCoroutine<int>::HandleType;

        DirectXSchedulingCoroutine() = delete;

        std::string m_name;
        DirectXSchedulingCoroutine(std::string name) :
            m_name(name) {

        }

        int get();

        bool await_ready();

        void await_suspend(std::experimental::coroutine_handle<> awaiting);

        auto await_resume();

        void resume();

        bool done();

        struct promise_type : public BaseCoroutine<int>::promise_type {

            auto get_return_object();

            auto initial_suspend();

        };

    };

}
