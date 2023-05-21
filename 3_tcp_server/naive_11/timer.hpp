#pragma once

#include <queue>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <memory>

#include <ctime>

#include "HTTPconnection.hpp"

/**
 * @brief 维护一个类似最小堆的结构，需要直接访问
 * 
 */

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;
//typedef std::unique_ptr<HTTPconnection> HTTPconnection_Ptr;

struct TimerNode {
    int id;             //用来标记定时器
    TimeStamp expire;   //设置过期时间
    TimeoutCallBack cb; //设置一个回调函数用来方便删除定时器时将对应的HTTP连接关闭

    //需要的功能可以自己设定
    bool operator<(const TimerNode& t) {
        return expire < t.expire;
    }
};


class TimerManager {
using SP_TimerNode = std::shared_ptr<TimerNode>;

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int,size_t> ref_; //映射一个fd对应的定时器在heap_中的位置 // fd : index

private:
    void delNode(size_t index) {
        /* 删除指定位置的结点 */
        assert(!heap_.empty() && index >= 0 && index < heap_.size());
        /* 将要删除的结点换到队尾，然后调整堆 */
        size_t i = index;
        size_t n = heap_.size() - 1;
        assert(i <= n);
        if(i < n) {
            swapNode(i, n);
            if(!shiftdown(i, n)) {
                shiftup(i);
            }
        }
        /* 队尾元素删除 */
        ref_.erase(heap_.back().id);
        heap_.pop_back();
    }
    void shiftup(size_t i) {
        assert(i >= 0 && i < heap_.size());
        size_t j = (i - 1) / 2;
        while(j >= 0) {
            if(heap_[j] < heap_[i]) { break; }
            swapNode(i, j);
            i = j;
            j = (i - 1) / 2;
        }
    }

    bool shiftdown(size_t index,size_t n) {
        assert(index >= 0 && index < heap_.size());
        assert(n >= 0 && n <= heap_.size());
        size_t i = index;
        size_t j = i * 2 + 1;
        while(j < n) {
            if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
            if(heap_[i] < heap_[j]) break;
            swapNode(i, j);
            i = j;
            j = i * 2 + 1;
        }
        return i > index;
    }

    void swapNode(size_t i,size_t j) {
        assert(i >= 0 && i < heap_.size());
        assert(j >= 0 && j < heap_.size());
        std::swap(heap_[i], heap_[j]);
        ref_[heap_[i].id] = i;
        ref_[heap_[j].id] = j;
    }

public:
    TimerManager() {heap_.reserve(64);}
    ~TimerManager() {clear();}

public:
    //设置定时器 
    void AddTimer(int id, int timeout, const TimeoutCallBack& cb) {
        assert(id >= 0);
        size_t i;
        if(ref_.count(id) == 0) {
            /* 新节点：堆尾插入，调整堆 */
            i = heap_.size();
            ref_[id] = i;
            heap_.push_back({id, Clock::now() + MS(timeout), cb});
            shiftup(i);
        } 
        else {
            /* 已有结点：调整堆 */
            i = ref_[id];
            heap_[i].expire = Clock::now() + MS(timeout);
            heap_[i].cb = cb;
            if(!shiftdown(i, heap_.size())) {
                shiftup(i);
            }
        }
    }

    //处理过期的定时器，核心
    void HandleExpiredEvent() {
        /* 清除超时结点 */
        if(heap_.empty()) {
            return;
        }
        while(!heap_.empty()) {
            TimerNode node = heap_.front();
            if(std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) { 
                break; 
            }
            node.cb(); // 启用回调函数关闭连接
            Pop();
        }
    }

    //下一次处理过期定时器的时间, 返回时间
    int GetNextHandle() {
        HandleExpiredEvent();
        size_t res = -1;
        if(!heap_.empty()) {
            res = std::chrono::duration_cast<MS>(heap_.front().expire - Clock::now()).count();
            if( res < 0) { res = 0; }
        }
        return res;
    }

    void Update(int id, int timeout) { // 只增加
        /* 调整指定id的结点 */
        assert(!heap_.empty() && ref_.count(id) > 0);
        heap_[ref_[id]].expire = Clock::now() + MS(timeout);;
        shiftdown(ref_[id], heap_.size());
    }

    //删除制定id节点，并且用指针触发处理函数
    void Work(int id) {
        /* 删除指定id结点，并触发回调函数 */
        if(heap_.empty() || ref_.count(id) == 0) {
            return;
        }
        size_t i = ref_[id];
        TimerNode node = heap_[i];
        node.cb();
        delNode(i);
    }

    void Pop() {
        assert(!heap_.empty());
        delNode(0); // ?
    }

    void Clear() {
        ref_.clear();
        heap_.clear();
    }
};





















class TimerManager{
    typedef std::shared_ptr<TimerNode> SP_TimerNode;
public:
    TimerManager() {heap_.reserve(64);}
    ~TimerManager() {clear();}
    //设置定时器 
    void addTimer(int id,int timeout,const TimeoutCallBack& cb);
    //处理过期的定时器
    void handle_expired_event();
    //下一次处理过期定时器的时间
    int getNextHandle();

    void update(int id,int timeout);
    //删除制定id节点，并且用指针触发处理函数
    void work(int id);

    void pop();
    void clear();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t index,size_t n);
    void swapNode_(size_t i,size_t j);

    std::vector<TimerNode>heap_;
    std::unordered_map<int,size_t>ref_;//映射一个fd对应的定时器在heap_中的位置
};
