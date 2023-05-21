#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>

#include "noncopyable.hpp"

class ThreadPool : public NonCopyable{
private:
    bool m_stop;
    std::vector<std::thread> m_thread;
    std::queue<std::function<void()>> tasks; // 
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit ThreadPool(size_t threadNumber):m_stop(false){
        for(size_t i=0;i<threadNumber;++i)
        {
            m_thread.emplace_back(
                [this](){
                    for(;;) // 每个线程在此执行循环任务！！
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lk(m_mutex);
                            m_cv.wait(lk, [this](){ return m_stop || !tasks.empty();}); // 没有任务就在这里卡住了
                            if (m_stop && tasks.empty())
                                return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        } // 放锁
                        task(); // 执行 
                    }
                }
            );
        }
    }
    // 继承NonCopyable 
    // ThreadPool(const ThreadPool &) = delete;
    // ThreadPool & operator=(const ThreadPool &) = delete;
    // 编译器自动删除 move
    // ThreadPool(ThreadPool &&) = delete;
    // ThreadPool & operator=(ThreadPool &&) = delete;

    ~ThreadPool(){
        { //次花括号用于管理生命周期，结束后放锁 m_mutex
            std::unique_lock<std::mutex> lk(m_mutex);
            m_stop=true;
            // 等价于 lk.unlock();
        }
        m_cv.notify_all();
        for(auto& threads:m_thread) {
            threads.join();
        }
    }

public:
    /**
     * @brief 向线程池对象提交一个任务
     * 
     * 使用返回的future来得到完成的返回值
     * 
     * @tparam F 
     * @tparam Args 
     * @param f 
     * @param args 
     * @return std::future<decltype(f(args...))> 
     */
    template<typename F,typename... Args>
    auto Submit(F&& f,Args&&... args) -> std::future<decltype(f(args...))> {
        // 一个指向 packaged_task<>的智能指针，任务可能被多出应用
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        ); 
        {
            std::unique_lock<std::mutex>lk(m_mutex);
            if(m_stop) throw std::runtime_error("submit on stopped ThreadPool");
            // *task_ptr 是 packaged_task<>
            // (*task_ptr)() 是一个函数返回类型 
            tasks.emplace([task_ptr](){ (*task_ptr)(); }); // function<void()>
        }
        m_cv.notify_one();
        return task_ptr->get_future();
    }
};

