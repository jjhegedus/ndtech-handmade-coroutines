#pragma once
#include <experimental\coroutine>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "VoidCoroutine.h"

using namespace std;
using namespace std::experimental;
using namespace std::chrono_literals;

template<typename T>
class ResumableDataProducingCoroutine {

public:

    struct promise_type {
        T m_value;
        ResumableDataProducingCoroutine get_return_object() {
            return ResumableDataProducingCoroutine(HandleType::from_promise(*this));
        }

        auto initial_suspend() { return std::experimental::suspend_never{}; }

        std::experimental::suspend_always final_suspend() { return {}; }

        void return_value(T value) {
            m_value = value;
        }

    };

    using HandleType = std::experimental::coroutine_handle<promise_type>;
    HandleType m_coroutine = nullptr;

    explicit ResumableDataProducingCoroutine(std::experimental::coroutine_handle<promise_type> coroutine)
        : m_coroutine(coroutine) {
    }

    ~ResumableDataProducingCoroutine() {
        if (m_coroutine) { m_coroutine.destroy(); }
    }

    ResumableDataProducingCoroutine() = default;

    ResumableDataProducingCoroutine(ResumableDataProducingCoroutine const&) = delete;

    ResumableDataProducingCoroutine& operator=(ResumableDataProducingCoroutine const&) = delete;

    ResumableDataProducingCoroutine(ResumableDataProducingCoroutine&& other)
        :m_coroutine(other.m_coroutine) {
        other.m_coroutine = nullptr;
    }

    ResumableDataProducingCoroutine& operator=(ResumableDataProducingCoroutine&& other) {
        if (&other != this) {
            m_coroutine = other.m_coroutine;
            other.m_coroutine = nullptr;
        }
        return *this;
    }

    T get() {
        return m_coroutine.promise().m_value;
    }

    struct awaiter;

    awaiter operator co_await() noexcept {
        awaiter returnObject = awaiter{ *this };

        return returnObject;
    }

    struct awaiter {
        ResumableDataProducingCoroutine& m_resumable;

        awaiter(ResumableDataProducingCoroutine& resumable)  noexcept
            :m_resumable(resumable) {

        }

        bool await_ready() {
            cout << "ResumableDataProducingCoroutine::await_ready\n";
            return false;
        }

        void await_resume() {
            cout << "ResumableDataProducingCoroutine::await_resume\n";
        }

        void await_suspend(coroutine_handle<> waitingCoroutine) {
            cout << "ResumableDataProducingCoroutine::await_suspend\n";
            m_resumable.m_waitingCoroutine = waitingCoroutine;
        }
    };

private:
    coroutine_handle<> m_waitingCoroutine;

};