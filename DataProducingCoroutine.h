#pragma once

#include <experimental\coroutine>

template<typename T>
class DataProducingCoroutine
{
public:
    struct promise_type
    {
        T m_value;
        DataProducingCoroutine get_return_object() {
            return DataProducingCoroutine(HandleType::from_promise(*this));
        }

        std::experimental::suspend_never initial_suspend() { return {}; }

        std::experimental::suspend_always final_suspend() { return {}; }

        void return_value(T value) {
            m_value = value;
        }

        void unhandled_exception() {}
    };


    using HandleType = std::experimental::coroutine_handle<promise_type>;
    HandleType m_coroutineHandle;

    DataProducingCoroutine(HandleType h)
        : m_coroutineHandle(h) {

    }

    T get() {
        if (!m_coroutineHandle.done()) {
            m_coroutineHandle.resume();
        }
        else {
            return m_coroutineHandle.promise().m_value;
        }
    }

    bool await_ready() {
        cout << "AwaitableDataProducingCoroutine::await_ready\n";
        return m_coroutineHandle.done();
    }

    auto await_resume() {
        cout << "AwaitableDataProducingCoroutine::await_resume\n";
        const auto returnValue = m_coroutineHandle.promise().m_value;
        return returnValue;
    }

    void await_suspend(std::experimental::coroutine_handle<> waitingCoroutine) {
        cout << "AwaitableDataProducingCoroutine::await_suspend\n";
        m_coroutineHandle.resume();
        waitingCoroutine.resume();
    }
};