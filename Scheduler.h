#pragma once

#include "pch.h"
#include "Utilities.h"
#include "VoidCoroutine.h"
#include <vector>
#include <functional>
#include <utility>
#include <chrono>
#include <condition_variable>
#include <algorithm>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace ndtech {

    struct Scheduler {

        Scheduler()
            :m_wakeTime(system_clock::now()) {
            m_cache_line_size = Utilities::cache_line_size();
            m_thread = std::thread{ &Scheduler::Run, this };
        }

        Scheduler(std::thread thread)
            :m_thread(std::move(thread)),
            m_wakeTime(system_clock::now()) {
            m_cache_line_size = Utilities::cache_line_size();
        }

        void Run() {
            while (true) {

                std::unique_lock<std::mutex> lockGuard(m_waitMutex);

                while (m_wakeTime > system_clock::now()) {
                    m_conditionVariable.wait_until(lockGuard, m_wakeTime, [this]() {return m_wakeTime > system_clock::now(); });
                }

                ProcessReadyTasks();

            }

        }

        void ProcessReadyTasks() {
            std::lock_guard lockGuard(m_tasksMutex);
            auto beginProcessingTime = system_clock::now();
            std::for_each(
                m_tasks.begin(),
                std::find_if(
                    m_tasks.begin(),
                    m_tasks.end(),
                    // check if the scheduled time for the task is now or past
                    [this, beginProcessingTime](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
                bool returnValue = (task.second > beginProcessingTime);
                return returnValue;
            }),
                // run the task
                [](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
                task.first();
            });

            m_tasks.erase(
                m_tasks.begin(),
                std::find_if(
                    m_tasks.begin(),
                    m_tasks.end(),
                    // check if the scheduled time for the task is now or past
                    [this, beginProcessingTime](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
                return task.second > beginProcessingTime;
            })
            );

            if (m_tasks.size() > 0) {
                m_wakeTime = m_tasks[0].second;
            }
            else {
                m_wakeTime = system_clock::now() + 100ms;
            }
        }

        void AddTask(std::pair<std::function<void(void)>, time_point<system_clock>> task) {

            { // to scope the lock guard
                std::lock_guard<std::mutex> guard(m_tasksMutex);
                m_tasks.push_back(task);

                if (task.second < m_wakeTime) {
                    m_wakeTime = task.second;
                }

                std::sort(
                    m_tasks.begin(),
                    m_tasks.end(),
                    [](std::pair<std::function<void(void)>, time_point<system_clock>> l, std::pair<std::function<void(void)>, time_point<system_clock>> r) {
                    return l.second < r.second;
                });
            }

            if (m_wakeTime <= system_clock::now()) {
                // signal the thread to wake
                m_conditionVariable.notify_all();
            }
        }

        void AddTask(std::function<void(void)> taskFunction) {

            std::pair<std::function<void(void)>, time_point<system_clock>> task{ taskFunction, system_clock::now() };

            { // to scope the lock guard
                std::lock_guard<std::mutex> guard(m_tasksMutex);
                m_tasks.push_back(task);

                if (task.second < m_wakeTime) {
                    m_wakeTime = task.second;
                }

                std::sort(
                    m_tasks.begin(),
                    m_tasks.end(),
                    [](std::pair<std::function<void(void)>, time_point<system_clock>> l, std::pair<std::function<void(void)>, time_point<system_clock>> r) {
                    return l.second < r.second;
                });
            }

            if (m_wakeTime <= system_clock::now()) {
                // signal the thread to wake
                m_conditionVariable.notify_all();
            }
        }

    private:
        size_t                                                                              m_cache_line_size;
        std::thread                                                                         m_thread;
        std::mutex                                                                          m_tasksMutex;
        std::vector<std::pair<std::function<void(void)>, time_point<system_clock>>>         m_tasks;
        time_point<system_clock>                                                            m_wakeTime = system_clock::now() + 100ms;
        std::mutex                                                                          m_waitMutex;
        std::condition_variable                                                             m_conditionVariable;
    };

}