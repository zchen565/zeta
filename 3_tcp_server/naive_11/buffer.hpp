#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <iostream>

#include <cstring>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>

#include "noncopyable.hpp"
/**
 * @brief 每一个链接配一个缓冲？
 * 
 */
class Buffer : public NonCopyable{
private: //data
    // 缓存实体
    std::vector<char> buffer_; //方便自动扩容，用string也行
    std::atomic<std::size_t> read_pos_, write_pos_; //读写指针

public: // class basic
    Buffer(int init_size=1024): buffer_(init_size), read_pos_(0), write_pos_(0){}
    // Buffer(const Buffer&) = delete;
    // Buffer& operator=(const Buffer&) = delete;
    ~Buffer() = default;

private: // private函数小写开头，和go保持一致
    char* beginPtr() {
        //return &*buffer_.begin();
        //return std::addressof(*buffer_.begin());
        //return std::data(buffer_);
        return buffer_.data(); // cpp11不能用于const容器
    }

    const char* beginPtr() const {
        return buffer_.data();
    }

    void allocateSpace(size_t len) {
        if (WriteableBytes() + ReadBytes() < len) {
            buffer_.resize(write_pos_ + len + 1);
        } else {
            // set to 0
            size_t readable = ReadableBytes();
            std::copy(beginPtr()+read_pos_, beginPtr()+write_pos_, beginPtr());
            read_pos_ = 0;
            write_pos_ = readable;
            assert(readable == ReadableBytes());        
        }   
    }

public:
    /**
     * @brief 指针相关
     * 
     */

    //获取字节数
    size_t WriteableBytes() const {
        return buffer_.size() - write_pos_; // pos都是下一个
    }
    size_t ReadableBytes() const {
        return write_pos_ - read_pos_; // -1 ?
    }
    size_t ReadBytes() const {
        return read_pos_;
    }
    //当前指针
    const char* CurReadPtr() const {
        return beginPtr() + read_pos_;
    }
    const char* CurWritePtrConst() const {
        return beginPtr() + write_pos_;
    }
    char* CurWritePtr() {
        return beginPtr() + write_pos_;
    }

    //update指针
    void UpdateReadPtr(size_t len) {
        assert(len <= ReadableBytes()); // 运行时编译器会删除
        read_pos_ += len;
    }
    void UpdateReadPtrToEnd(const char* end) {
        assert(end >= CurReadPtr());
        UpdateReadPtr(end-CurReadPtr()); 
    }
    void UpdateWritePtr(size_t len) {
        assert(len <= WriteableBytes());
        write_pos_ += len;
    }

    //初始化指针
    void InitPtr(){
        read_pos_ = 0;
        write_pos_ = 0;
        std::memset(buffer_.data(), 0, buffer_.size()); // 请勿使用bzero
        //多次一举？
    }

    /**
     * @brief 缓冲相关
     * 
     */
    //保证将数据写入缓冲区
    void EnsureWriteable(size_t len) {
        if(WriteableBytes() < len) {
            allocateSpace(len);
        } 
        assert(WriteableBytes() >= len);
    }   
    //将数据写入到缓冲区
    void Append(const char* str,size_t len) {
        assert(str);
        EnsureWriteable(len);
        std::copy(str, str+len, CurWritePtr());
        UpdateWritePtr(len);
    }
    void Append(const std::string& str) {
        Append(str.data(), str.length());
    }
    void Append(const void* data,size_t len) {
        assert(data);
        Append(static_cast<const char*>(data), len);
    }
    void Append(const Buffer& buffer) {
        Append(buffer.CurReadPtr(), buffer.ReadableBytes());
    }

    /**
     * @brief IO操作
     * 
     */
    ssize_t ReadFd(int fd,int* Errno) {
        char buff[65535]; //暂时的缓冲区, 用于存储无法立即写入缓冲的剩余数据
        struct iovec iov[2];
        const size_t writable = WriteableBytes();

        iov[0].iov_base= beginPtr() + write_pos_;
        iov[0].iov_len=writable;
        iov[1].iov_base=buff;
        iov[1].iov_len=sizeof(buff);

        // readv 为系统调用
        const ssize_t len = readv(fd,iov,2);
        if(len<0) {
            //std::cout<<"从fd读取数据失败！"<<std::endl;
            *Errno=errno;
        } else if(static_cast<size_t>(len)<=writable) {
            write_pos_ += len;
        } else {
            write_pos_ =buffer_.size();
            Append(buff,len-writable);
        }
        return len;
    }

    ssize_t WriteFd(int fd,int* Errno) {
        size_t readSize = ReadableBytes();
        ssize_t len=write(fd,CurReadPtr(),readSize);
        if(len<0) {
            //std::cout<<"往fd写入数据失败！"<<std::endl;
            *Errno=errno;
            return len;
        }
        read_pos_+=len;
        return len;
    }

    /**
     * @brief 转化
     * 
     */
    std::string Buffer::AlltoStr()
    {
        std::string str(CurReadPtr(), ReadableBytes());
        InitPtr();
        return str;
    }

    /**
     * @brief test
     * 
     */
    void PrintContent()
    {
        std::cout << "pointer location info:"<<read_pos_<<" "<<write_pos_<<std::endl;
        for(int i=read_pos_;i<=write_pos_;++i)
        {
            std::cout<<buffer_[i]<<" ";
        }
        std::cout<<std::endl;
    }

};