#pragma once

#include <sstream>

namespace ndtech {

    struct BaserCoroutine {
        struct promise_type {
            int value;

            promise_type() {
                OutputDebugStringA("Promise created\n");
            }

            ~promise_type() {
                OutputDebugStringA("promise_type destructor\n");
            }

            auto get_return_object() {
                OutputDebugStringA("return a BaserCoroutine\n");
                return BaserCoroutine{ handle_type::from_promise(*this) };
            }

            auto initial_suspend() {
                OutputDebugStringA("Started the coroutine, now suspend\n");
                return std::experimental::suspend_always{};
            }

            auto return_value(int v) {
                std::stringstream ss;
                ss << "Got an answer of " << v << std::endl;
                OutputDebugStringA(ss.str().c_str());
                value = v;
                return std::experimental::suspend_never{};
            }

            auto final_suspend() {
                OutputDebugStringA("Finished the coroutine\n");
                return std::experimental::suspend_always{};
            }

            void unhandled_exception() {
                std::exit(1);
            }

            int getValue() {
                return value;
            }

        };
        using handle_type = std::experimental::coroutine_handle<promise_type>;

        std::shared_ptr<int> value;
        handle_type coroutine_handle;
        std::experimental::coroutine_handle<> m_awaiting;

        BaserCoroutine(handle_type h)
            :coroutine_handle(h) {
            OutputDebugStringA("Created BaserCoroutine object\n");
        }

        BaserCoroutine(const BaserCoroutine &) = delete;

        BaserCoroutine(BaserCoroutine &&r)
            :coroutine_handle(r.coroutine_handle) {
            OutputDebugStringA("BaserCoroutine moved, leaving behind an empty, unusable husk\n");
            r.coroutine_handle = nullptr;
        }

        ~BaserCoroutine() {
            OutputDebugStringA("BaserCoroutine destructor\n");
            if (coroutine_handle) coroutine_handle.destroy();
        }

        BaserCoroutine &operator = (const BaserCoroutine &) = delete;

        BaserCoroutine &operator = (BaserCoroutine &&r) {
            coroutine_handle = r.coroutine_handle;
            r.coroutine_handle = nullptr;
            return *this;
        }

        int get() {
            OutputDebugStringA("We got asked for the return value...\n");
            coroutine_handle.resume();
            return coroutine_handle.promise().value;
        }

        bool await_ready() {
            const auto ready = this->coroutine_handle.done();
#ifdef _DEBUG
            std::stringstream ss;
            ss << "ResumableCoroutine::await_ready(): " << (ready ? "is ready" : "isn't ready") << std::endl;
#endif
            DebugOutput(ss.str().c_str());

            return this->coroutine_handle.done();
        }

        void await_suspend(std::experimental::coroutine_handle<> awaiting) {
            //DebugOutput("ResumableCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaitable\n");
            //this->coroutine_handle.resume();
            //DebugOutput("ResumableCoroutine::await_suspent(std::experimental::coroutin_handle<> awaiting):: About to resume the awaiter\n");
            //awaiting.resume();
            m_awaiting = awaiting;
            this->coroutine_handle.resume();
        }

        auto await_resume() {
            const auto returnValue = this->coroutine_handle.promise().getValue();
#ifdef _DEBUG
            std::stringstream ss;
            ss << "ResumableCoroutine::await_resume(): returning value = " << returnValue << std::endl;
#endif
            DebugOutput(ss.str().c_str());

            return returnValue;


            //            if (this->coroutine_handle.done()) {
            //                const auto returnValue = this->coroutine_handle.promise().getValue();
            //#ifdef _DEBUG
            //                std::stringstream ss;
            //                ss << "ResumableCoroutine::await_resume(): returning value = " << returnValue << std::endl;
            //#endif
            //                DebugOutput(ss.str().c_str());
            //
            //                return returnValue;
            //            } else {
            //                this->coroutine_handle.resume();
            //            }
        }


    };

}