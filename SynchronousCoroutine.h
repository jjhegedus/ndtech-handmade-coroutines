#pragma once

#include "BaseCoroutine.h"

namespace ndtech {
    
    template<typename T>
    struct SynchronousCoroutine : public BaseCoroutine<T> {
        using BaseCoroutine<T>::BaseCoroutine;
        using HandleType = typename BaseCoroutine<T>::HandleType;

        T get() {
            DebugOutput("SynchronousCoroutine::get()\n");
            return BaseCoroutine<T>::get();
        }

        struct promise_type : public BaseCoroutine<T>::promise_type {

            auto get_return_object() {
                DebugOutput("SynchronousCoroutine::promise_type::get_return_object()\n");
                return SynchronousCoroutine<T>{HandleType::from_promise(*this)};
            }

            auto initial_suspend() {
                DebugOutput("SynchronousCoroutine::promise_type::initial_suspend()\n");
                return std::experimental::suspend_never{};
            }

        };


    };

}