#include "fiber.h"

static bool debug = false;

namespace sylar {

static thread_local Fiber* t_fiber = nullptr;  // 当前协程

static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;  // 主协程

static thread_local Fiber* t_scheduler_fiber = nullptr;  // 调度协程


static std::atomic<uint64_t> s_fiber_id{0};  // 协程id

static std::atomic<uint64_t> s_fiber_count{0};   // 协程计数器

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

std::shared_ptr<Fiber> Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    std::shared_ptr<Fiber> main_fiber(new Fiber());
    t_thread_fiber = main_fiber;
    t_scheduler_fiber = main_fiber.ger();

    assert(t_fiber == main_fiber.get());
    return t_fiber->shared_from_this();
}

void Fiber::SetScheduler(Fiber* f) {
    t_scheduler_fiber = f;
}

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return (uint64_t)-1;
}

Fiber::Fiber(){
    SetThis(this);
    m_state = RUNNING;

    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    m_id = s_fiber_id++;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() main id = " << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler):
m_cb(cb), m_runInScheduler(run_in_scheduler) {
    m_state = READY;

    // 分配协程栈空间
    m_stacksize = stacksize ? stacksize : 128000;
    m_stack = malloc(m_stacksize);

    if (getcontext(&m_ctx)){
        std::cerr << "Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) failed\n";
        pthread_exit(nullptr);
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    m_id = s_fiber_id++;
    s_fiber_count++;
    if (debug) {
        std::cout << "Fiber::Fiber() child id = " << m_id << std::endl;
    }

}

Fiber::~Fiber() {
    s_fiber_count--;
    if (m_stack) {
        free(m_stack);
    }
    if (debug) {
        std::cout << "~Fiber(): id = " << m_id << std::endl;
    }
}

void Fiber::reset(std::function<void()> cb) {
    assert(m_stack != nullptr && m_state == TERM);

    m_state = READY;
    m_cb = cb;

    if (get_context(&m_ctx)){
        std::cerr << "reset() failed\n";
        pthread_exit(nullptr);
    }

    m_ctx.uc_link = nullptr
    m_ctx.uc_stack.ss_sp = m_stack; 
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

}

void Fiber::resume() {
    assert(m_state == READY);

    m_state = RUNNING;

    if (m_runInScheduler) {
        SetThis(this);
        if (swapcontext(&(t_scheduler_fiber->m_ctx), &m_ctx)) {
            std::cerr << "resume() to t_scheduler_fiber failed\n";
            pthread_exit(NULL);
        }
    }
    else {
        SetThis(this);
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
            std::cerr << "resume() to t_thread_fiber failed\n";
            pthread_exit(NULL);
        }
    }
}

}


