#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <iostream>
#include<memory>
#include <atomic>
#include <functional>
#include <cassert>
#include <ucontext.h>
#include <unistd.h>
#include <mutex>


// 协程的三种状态
namespace sylar {
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State {
        READY,  // 准备状态
        RUNNING,  // 运行状态
        TERM,  // 结束状态
    }
private:
    Fiber(); 

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

    ~Fiber();  // 析构函数

    void reset(std::function<void()> cb);  // 重置协程函数，并重置状态

    void resume();  // 将当前协程和正在执行的协程进行交换

    void yield();  // 让出当前协程

    uint64_t getId() const {return m_id; }  // 获取协程id

    State getState() const {return m_state; }  // 获取协程状态

public:
    static void SetThis(Fiber* f);  // 设置当前协程

    static Fiber::ptr GetThis();  // 获取当前协程

    static uint64_t TotalFibers();  // 获取当前协程总数

    static void MainFunc();  // 协程执行数

    static uint64_t GetFiberId();  // 获取当前协程id

private:
    uint64_t m_id = 0;  // 协程id

    uint64_t m_stacksize = 0;  // 协程栈大小

    State m_state = READY;  // 协程状态

    ucontext_t m_ctx;  // 协程上下文

    void* m_stack = nullptr;  // 协程栈

    std::function<void()> m_cb;  // 协程执行函数

    bool m_runInScheduler;  // 是否在调度器中运行

public:
    std::mutex m_mutex;  // 互斥锁

};

}
