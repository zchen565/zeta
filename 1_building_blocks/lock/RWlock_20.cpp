#include <unordered_map>
#include <iostream> // cout need a mutex to avoid parallel cout
#include <thread>
#include <mutex> // unique_lock
#include <shared_mutex>
#include <chrono>
#include <random>

// have a look at how <shared_mutex> is implemented


// simple map read
class RawMap {
public:
    RawMap() {
    };

    ~RawMap() {};

    // delete the 
private:
    // std::unordered_map<int, std::pair<std::shared_mutex, int>> inner;
    std::unordered_map<int, std::shared_mutex> rwmutex_; // init a shared_mutex
    std::unordered_map<int, int> content_;

public:
    int get(int k){ // read
        if(rwmutex_.find(k) == rwmutex_.end()){ // not found
            return -1;
        }
        std::shared_lock<std::shared_mutex> lck(rwmutex_[k]);
        return content_[k];
    }

    int set(int k, int v){ // write
        std::unique_lock<std::shared_mutex> lck(rwmutex_[k]);
        content_[k] += 1;
        return content_[k];
    }
    
    int erase(int k){ // write
        if(rwmutex_.find(k) == rwmutex_.end()){ // not found
            return -1;
        }
        std::unique_lock<std::shared_mutex> lck(rwmutex_[k]);
        content_.erase(k);
        rwmutex_.erase(k); // this might cause some problem ?
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
        rrth[i] = std::jthread(reader, i+1);
    }

    for(int i=0; i<10; i++){
        wwth[i] = std::jthread(writer, i+1);
    }

    // for(int i=0; i<10; i++){
    //     eeth[i] = std::jthread(eraser, 2);
    // }

    return 0;
}
