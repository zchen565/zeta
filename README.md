# zeta

There are currently 3 stages of this program, each unit can be tested separately. Latter unit will use the previous unit.

It is based on C++20 for the _20 files or objects (or by default).
C++11 will end up with _11 and C (pthread) will end up with _c.

1. building blocks (mutex/lock, threads, memory, coroutines ...)
2. encapsulation (further encap and import of other libs)
3. tcp server (build our tcp server with epoll, coroutine)
4. rpc server (build our rcp server with protobuf or self class)
5. head to distributed service (paxos, multi-paxos, raft)   
