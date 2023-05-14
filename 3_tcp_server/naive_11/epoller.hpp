#pragma once

#include<vector>

#include<sys/epoll.h> //epoll_ctl()
#include<fcntl.h> //fcntl()
#include<unistd.h> //close()
#include<assert.h>
#include<errno.h>

/**
 * @brief epoll封装
 * 
 * 查看 io_uring 
 * 
 */
class Epoller{
private:
    int epoller_fd_;//这是标志epoll的描述符
    std::vector<struct epoll_event> events_; //就绪的事件 //就绪事件一般不是双向链表吗？
    // Reminder:
    //     struct epoll_event {
    //         uint32_t events;    // 就绪事件的类型
    //         epoll_data_t data;  // 用户定义的数据 //用于事件触发找到fd
    //     };
public:
    explicit Epoller(int maxEvent=1024) 
        : epoller_fd_(epoll_create(512)), events_(maxEvent=1024){ // explicit 禁止隐士转换
        // epller_create()参数已被忽略，现在可以任意大
        assert(epoller_fd_ >= 0 && events_.size() > 0);
    }

    ~Epoller() {
        close(epoller_fd_);
    }

public:
    //将描述符fd加入epoll监控
    bool AddFd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0}; // {} 等效
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epoller_fd_, EPOLL_CTL_ADD, fd, &ev);
    }

    //修改描述符fd对应的事件
    bool ModFd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0}; // {} 等效
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epoller_fd_, EPOLL_CTL_MOD, fd, &ev);
    }
    //将描述符fd移除epoll的监控
    bool DelFd(int fd) {
        if(fd < 0) return false;
        epoll_event ev = {0}; // {} 等效
        return 0 == epoll_ctl(epoller_fd_, EPOLL_CTL_DEL, fd, &ev);
    }
    //用于返回监控的结果，成功时返回就绪的文件描述符的个数
    int Wait(int timeoutMs) {
        // events_已经被初始化为1024个不会有超过的
        // 调整相应参数进行测试
        return epoll_wait(epoller_fd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    }

    //获取fd的函数
    //在for循环中调用
    int GetEventFd(size_t i) const {
        assert(i < events_.size() && i >= 0);
        return events_[i].data.fd; 
    }
    //获取events的函数
    //同for，获取事件类型
    uint32_t GetEvents(size_t i) const { // 返回类型
        assert(i < events_.size() && i >= 0);
        return events_[i].events;
    }
};