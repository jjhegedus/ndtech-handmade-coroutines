#pragma once

#include <sstream>

namespace ndtech {

    template <typename T>
    struct nonresumable {
        struct promise_type;
        using handle_type = std::experimental::coroutine_handle<promise_type>;

        std::shared_ptr<T> value;
        handle_type coroutine_handle;

        nonresumable(handle_type h)
            :coroutine_handle(h) {
            OutputDebugStringA("Created nonresumable object\n");
        }

        nonresumable(const nonresumable &) = delete;

        nonresumable(nonresumable &&r)
            :coroutine_handle(r.coroutine_handle) {
            OutputDebugStringA("nonresumable moved, leaving behind an empty, unusable husk\n");
            r.coroutine_handle = nullptr;
        }

        ~nonresumable() {
            OutputDebugStringA("nonresumable destructor\n");
            if (coroutine_handle) coroutine_handle.destroy();
        }

        nonresumable &operator = (const nonresumable &) = delete;

        nonresumable &operator = (nonresumable &&r) {
            coroutine_handle = r.coroutine_handle;
            r.coroutine_handle = nullptr;
            return *this;
        }

        T get() {
            OutputDebugStringA("We got asked for the return value...\n");
            return coroutine_handle.promise().value;
        }

        struct promise_type {
            T value;

            promise_type() {
                OutputDebugStringA("Promise created\n");
            }

            ~promise_type() {
                OutputDebugStringA("promise_type destructor\n");
            }

            auto get_return_object() {
                OutputDebugStringA("return a nonresumable\n");
                return nonresumable<T>{handle_type::from_promise(*this)};
            }

            auto initial_suspend() {
                OutputDebugStringA("Started the coroutine\n");
                return std::experimental::suspend_never{};
            }

            auto return_value(T v) {
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

        };
    };

}