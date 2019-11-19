# concurrency

A dreiver typically provides 2 main APIs for clients: schedule (queues a job) and handle (does a job). 

- Schedule and handle operations could be synchronous or asynchronous.

- handler
    - In synchronous mode, handle always blocks until there is job to handle.
    - In asynchronous mode, they simply return.

- schedule
    - syn: if the job queue is full, schedulers waits until a handler has processes a job and there is available space in the job queue.
    - asy: they simply leave without scheduling.
