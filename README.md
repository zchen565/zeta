# zeta

There are currently 4 stages of this program, each unit can be tested separately. Latter unit will use the previous unit.

It is based on C++17. (We will avoid the garbage stuff in C++ 17)
It will be noted if there is use of C++20.

1. building blocks (mutex/lock, threads, memory, coroutines ...)
2. encapsulation (further encap and import of other libs)
3. tcp server (build our tcp server with epoll)
4. rpc server (build our rcp server with protobuf and self class)
5. head to paxos (paxos, multi-paxos, raft)
