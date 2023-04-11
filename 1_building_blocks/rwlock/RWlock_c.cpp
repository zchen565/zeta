
#include <pthread.h>
#include <stdio.h>
#include <unordered_map>

// for test
#include <iostream>
#include <thread> 
#include <mutex>

/**
 * @brief 局部锁的模板实现
 */
template<class T>
struct ScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex Mutex
     */
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    /**
     * @brief 析构函数,自动释放锁
     */
    ~ScopedLockImpl() {
        unlock();
    }

    /**
     * @brief 加锁
     */
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    /// mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 */
template<class T>
struct ReadScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    /**
     * @brief 析构函数,自动释放锁
     */
    ~ReadScopedLockImpl() {
        unlock();
    }

    /**
     * @brief 上读锁
     */
    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    /**
     * @brief 释放锁
     */
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    /// mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 */
template<class T>
struct WriteScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    /**
     * @brief 析构函数
     */
    ~WriteScopedLockImpl() {
        unlock();
    }

    /**
     * @brief 上写锁
     */
    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    /// Mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};

/**
 * @brief 读写互斥量
 */
class RWMutex {
public:

    /// 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    /// 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    /**
     * @brief 构造函数
     */
    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    
    /**
     * @brief 析构函数
     */
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
        printf("********\n");
    }

    /**
     * @brief 上读锁
     */
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    /**
     * @brief 上写锁
     */
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    /**
     * @brief 解锁
     */
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    /// 读写锁
    pthread_rwlock_t m_lock;
};



// user RWMutex

class RawMap {
public:
    RawMap() {
    };

    ~RawMap() {};

    // delete the 
private:
    // std::unordered_map<int, std::pair<std::shared_mutex, int>> inner;
    std::unordered_map<int, RWMutex> rwmutex_; // init a shared_mutex
    std::unordered_map<int, int> content_;

public:
    int get(int k){ // read
        if(rwmutex_.find(k) == rwmutex_.end()){ // not found
            return -1;
        }
        rwmutex_[k].rdlock();
        int temp = content_[k];
        rwmutex_[k].unlock();
        return temp;
    }

    int set(int k, int v){ // write
        // std::unique_lock<std::shared_mutex> lck(rwmutex_[k]);
        rwmutex_[k].wrlock();
        content_[k] += 1;
        int temp = content_[k];
        rwmutex_[k].unlock();
        return temp;
    }
    
    int erase(int k){ // write
        if(rwmutex_.find(k) == rwmutex_.end()){ // not found
            return -1;
        }
        rwmutex_[k].wrlock();
        content_.erase(k);
        rwmutex_.erase(k); 
        
        // this might cause some problem ?

        return 1;
    }

};




// init for test simplicty
RawMap counter;
std::mutex mtx;

// thread tester
void reader(int id){
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        std::cout << "reader #" << id << " get value " << counter.get(id) << "\n";
        // if(id == 2){
        //     std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        //     std::cout << "reader #" << id << " get value " << counter.get(id) << "\n";
        // }
    }
}

void writer(int id){
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        std::cout << "writer #" << id << " write value " << counter.set(id, rand()) << "\n";
        // if(id == 2){
        //     std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        //     std::cout << "writer #" << id << " write value " << counter.set(id, rand()) << "\n";
        // }
    }
}

void eraser(int id){
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10+ rand() % 3) );
        std::unique_lock<std::mutex> ulck(mtx);//cout也需要锁去保护, 否则输出乱序
        std::cout << "eraser #" << id << " erase value " << counter.erase(id) << "\n";
    }
}

int main(int argc, char const *argv[])
{
    /* code */

    std::jthread rrth[10];
    std::jthread wwth[10];
    std::jthread eeth[10];

    for(int i=0; i<10; i++){
        rrth[i] = std::jthread(reader, 2);
    }

    for(int i=0; i<10; i++){
        wwth[i] = std::jthread(writer, 2);
    }

    for(int i=0; i<10; i++){
        eeth[i] = std::jthread(eraser, 2);
    }

    return 0;
}








