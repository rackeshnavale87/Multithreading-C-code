# Token bucket - Multithreading : C implementation
Multithreading usage for implementing "client, tocken and servers managerment" (Tocken bucket system implementation).

This project implements the Tocken Bucket system implementation. 
[Lines of Code : 1200+]
- There are two server threads (can be increased to improve the performance)
- One Input packet thread
- One token thread

<img src="https://lh3.googleusercontent.com/4NkxGQWXVJK2VBVA1ahepMT0H2v4P1HC4PlTfn0Hn-HM50CAjaCi8JZpiwefFFFse_FWeg=s170" alt="Token Bucket" style="float:center;width:1024px;height:900px">


High level features :
- Signal management.
- Multhreading management.
- Mutex implementation.
- No deadlock & no busy waiting.
