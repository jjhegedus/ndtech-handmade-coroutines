#pragma once

#include <experimental\coroutine>
#ifdef _DEBUG
#include <sstream>
#include <debugapi.h>
#endif

//#ifdef _DEBUG
//#ifndef OUTPUT_DEBUG_STRINGS
//#define DebugOutput(cstr)
//#endif
//#endif

namespace ndtech {


    template<typename T>
    struct BaseCoroutine {
        struct promise_type;
        friend struct promise_type;
        using HandleType = std::experimental::coroutine_handle<promise_type>;

        BaseCoroutine(const promise_type &) = delete;

        BaseCoroutine(){};

        BaseCoroutine(BaseCoroutine &&bc)
            :m_coroutineHandle(bc.m_coroutineHandle) {
            DebugOutput("BaseCoroutine::BaseCoroutine(BaseCoroutine &&c)\n");
            bc.m_coroutineHandle = nullptr;
        }

        ~BaseCoroutine() {
            DebugOutput("BaseCoroutine::~BaseCoroutine()\n");
            if (m_coroutineHandle) m_coroutineHandle.destroy();
        }

        BaseCoroutine &operator = (const BaseCoroutine &) = delete;

        BaseCoroutine &operator = (BaseCoroutine &&bc) {
            m_coroutineHandle = bc.m_coroutineHandle;
            bc.m_coroutineHandle = nullptr;
            return *this;
        }


        struct promise_type {
            friend struct BaseCoroutine;

            promise_type() {
                DebugOutput("BaseCoroutine::promise_type::promise_type()\n");
            }

            ~promise_type() {
                DebugOutput("BaseCoroutine::promise_type::~promise_type()\n");
            }

            auto return_value(T v) {
//#ifdef _DEBUG
//                std::stringstream ss;
//                ss << "BaseCoroutine::promise_type::return_value(T v) returning " << v << std::endl;
//                DebugOutput(ss.str().c_str());
//#endif

                DebugOutput("BaseCoroutine::promise_type::return_value(T v)\n");
                value = v;
                //return std::experimental::suspend_never{};
                return value;
            }

            auto final_suspend() {
                DebugOutput("BaseCoroutine::promise_type::final_suspend()\n");
                return std::experimental::suspend_always{};
            }

            void unhandled_exception() {
                DebugOutput("BaseCoroutine::promise_type::unhandled_exception()\n");
                std::exit(1);
            }

            T getValue() {
                return value;
            }

        private:
            T value;

        };


    protected:
        HandleType m_coroutineHandle;

        T get() {
            DebugOutput("BaseCoroutine::get()\n");
            return m_coroutineHandle.promise().value;
        }

        BaseCoroutine(HandleType h)
            : m_coroutineHandle(h) {
            DebugOutput("BaseCoroutine::BaseCoroutine(HandleType h)\n");
        }

    };

}