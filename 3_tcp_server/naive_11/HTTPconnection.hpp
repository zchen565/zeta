#pragma once
#include <iostream>

#include <arpa/inet.h> //sockaddr_in
#include <sys/uio.h> //readv/writev
#include <sys/types.h>
#include <assert.h>

#include "buffer.hpp"
#include "HTTPrequest.hpp"
#include "HTTPresponse.hpp"

class HTTPconnection{
private:
    int fd_;                  //HTTP连接对应的描述符
    // Reminder:
    // struct sockaddr_in {
    //     sa_family_t sin_family; // 地址族（通常为 AF_INET）
    //     in_port_t sin_port; // 16 位端口号（使用网络字节顺序）
    //     struct in_addr sin_addr; // IPv4 地址
    //     char sin_zero[8]; // 填充字段，通常设置为 0
    // };
    struct sockaddr_in addr_;

    bool isClose_;            //标记是否关闭连接

    int iovCnt_;
    struct iovec iov_[2];

    Buffer read_buffer_;       //读缓冲区
    Buffer write_buffer_;      //写缓冲区

    HTTPrequest request_;
    HTTPresponse response_;

public:
    static bool IsET_; // what is this ?
    static const char* SrcDir_; // 挂载地点
    static std::atomic<int> UserCount_;

public:
    HTTPconnection() {
        fd_ = -1;
        addr_ = {0};
        isClose_ = true;
    }
    ~HTTPconnection() {
        CloseHTTPConn();
    }

public:
    void InitHTTPConn(int fd, const sockaddr_in& addr) {
        assert(fd > 0);
        UserCount_++;
        addr_ = addr;
        fd_ = fd;
        write_buffer_.InitPtr();
        read_buffer_.InitPtr();
        isClose_ = false;
    }

    //关闭HTTP连接的接口
    void CloseHTTPConn() { // the callback in timer.hpp
        response_.UnmapFile();
        if(isClose_ == false){
            isClose_ = true;
            UserCount_ --; // atomic重载
            close(fd_);
        }
    }

    //定义处理该HTTP连接的接口，主要分为request的解析和response的生成
    bool HandleHTTPConn() {
        request_.Init();
        if(read_buffer_.ReadableBytes() <= 0) {
            return false;
        } else if (request_.Parse(read_buffer_)) {
            response_.Init(SrcDir_, request_.Path(), request_.IsKeepAlive(), 200);
        } else {
            std::cout << "400! " << std::endl;
            response_.Init(SrcDir_, request_.Path(), false, 400);
        }

        // 
        response_.MakeResponse(write_buffer_);

        //响应头
        iov_[0].iov_base = const_cast<char*>(write_buffer_.CurReadPtr());
        iov_[0].iov_len = write_buffer_.ReadableBytes();
        iovCnt_ = 1;

        //文件
        if(response_.FileLen() > 0 && response_.File()) {
            iov_[1].iov_base = response_.File();
            iov_[1].iov_len = response_.FileLen();
            iovCnt_ = 2;
        }
        return true;
    }


    //每个连接中定义的对缓冲区的读写接口
    ssize_t ReadBuffer(int* saveErrno) { // errno
        ssize_t len = -1;
        do {
            len = read_buffer_.ReadFd(fd_, saveErrno);
            if(len <= 0) break;
        } while (IsET_); // what is this
        return len;
    }

    ssize_t WriteBuffer(int* saveErrno) {
        ssize_t len = -1;
        do {
            len = writev(fd_, iov_, iovCnt_);
            if(len <= 0){
                *saveErrno = errno;
                break;
            }
            if(iov_[0].iov_len + iov_[1].iov_len == 0) {
                break; // END
            } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
                iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
                iov_[1].iov_len -= (len - iov_[0].iov_len);
                if(iov_[0].iov_len) {
                    write_buffer_.InitPtr();
                    iov_[0].iov_len = 0;
                }
            } else {
                iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
                iov_[0].iov_len -= len; 
                write_buffer_.UpdateReadPtr(len);
            }
        } while (IsET_ || WriteBytes() > 10240);
        return len;
    }

    //信息获取
    const char* GetIP() const {
        return inet_ntoa(addr_.sin_addr);
    }
    int GetPort() const {
        return addr_.sin_port;
    }
    int GetFd() const {
        return fd_;
    }
    sockaddr_in GetAddr() const {
        return addr_;
    }

    //其他
    int WriteBytes() {
        return iov_[1].iov_len+iov_[0].iov_len; // 返回可写长度
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

};
