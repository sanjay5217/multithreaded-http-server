# HTTP-Server

A multithreaded HTTP/1.1 server built from scratch in C++17, using raw POSIX sockets with no external networking libraries. This project was a deep dive into C++ OOP, RAII resource management, design principles, systems programming including sockets and HTTP parsing, and concurrent request handling via a thread pool and job queue.

I divided this project into 2 phases.

- **Phase 1:** Core HTTP parsing pipeline with single-threaded request handling, covering six custom endpoints
- **Phase 2:** Thread pool, kqueue event loop, non-blocking I/O

## Phase 1

**Phase 1 Blueprint:**

![diagram](/images/image-1.png)

Phase 1 is a single-threaded server that parses HTTP requests sent via `curl localhost:PORT` and replies back. The main objective was to get the TCP socket working by reading and parsing raw HTTP bytes and sending messages back. To make the task more challenging, I designed six unique endpoints to push the server's routing and handler design.

- `GET /health`: returns the status of the server

  ```
  $ curl http://localhost:8080/health
  { Status: OK }
  ```

- `GET /stat`: returns the number of requests served

  ```
  curl http://localhost:8080/stat
  { Requests Served: 2 }
  ```

- `GET /header`: returns the headers of the request

  ```
  curl http://localhost:8080/header -H "Custom: hello"
  { path: /header }
  { Host: localhost:8080 }
  { method: GET }
  { Accept: */* }
  { version: HTTP/1.1 }
  { User-Agent: curl/8.7.1 }
  { Custom: hello }
  ```

- `GET /compute`: returns the CPU time of a Fibonacci calculation

  ```
  curl http://localhost:8080/compute
  { N: 44, Result: 1134903170, Duration: 3.34752 }
  ```

- `POST /echo`: returns the message body (similar to bash's `echo`)

  ```
  curl http://localhost:8080/echo -d "Hello World"
  Hello World
  ```

- `POST /static`: returns the contents of a file (similar to bash's `cat`)

  ```
  curl http://localhost:8080/static -d input.txt
  Hello world!
  This is a test file.
  Used for StaticHandler testing.
  End of file.
  ```

  _Note: The path is relative to whichever directory `./server` is run from._

To do this, I constructed a Router class with an `exec_handler` method. This followed the Command design pattern, where handler classes use polymorphism to return an `httpResponse` based on the request path.

![diagram](/images/image-2.png)

## Phase 2

Phase 2 introduces concurrency to address two inefficiencies in the single-threaded server:

1. Requests are handled sequentially. This means a slow or expensive request blocks every client behind it.
2. The server uses blocking I/O; `recv()` freezes the thread until bytes arrive.

**Version 1: Thread Pool**

To address the first problem, I refactored the server to use a fixed thread pool with a condition-variable-driven job queue. At startup, N worker threads are spawned and sleep on a condition variable. When the main thread accepts a client fd and pushes it onto the queue, one worker wakes up, handles the full request, and returns to sleep. Requests are now processed concurrently, so a slow request like `/compute` no longer blocks `/health`.

![diagram](/images/image-3.png)

**Version 2: Non-Blocking I/O and Event Loops**

Since context switches are costly, a thread-per-client architecture is not system-efficient. I decided to explore event loops to minimize that overhead. Additionally, since `recv()` is blocking, I switched to a non-blocking architecture using `kqueue` and `kevent`, which are native to macOS. This was the most complex part of the project. It required RAII management within threads, significant refactoring of the existing architecture, and careful handling of per-client state depending on whether a client is waiting for I/O or actively being processed.

![diagram](/images/image-4.png)

Another model I considered was a **shared executor pool** for handlers.

![diagram](/images/image-5.png)

Essentially, handlers would also be threaded, with a separate thread pool executing them independently. The main advantage would have been full isolation between clients, so no client could stall behind another's slow request. However, I chose not to pursue this due to the added complexity in request flow. Originally, all I had to do was push the client fd to one pool and everything was handled. With this model, I would need to push the client to a pool, retrieve the request object, then push it to a second pool that handles the router execution. But this would have been unnecessary overhead for fast endpoints like `/health` and would only optimize the stalling cases with `/compute`.

## Benchmarking Results

I used `wrk` to benchmark performance across the three models: single-thread, client-per-thread, and non-blocking kqueue. I also tested two configurations of the kqueue version, one with 1 worker thread and one with 10. I ran two distinct tests:

1. **Test 1:** Pure throughput: hammer `/health` with 100 concurrent connections for 10 seconds.

   ```
   wrk -t4 -c100 -d10s --latency http://localhost:8080/health
   ```

   **Results**

   | Architecture                   | req/s | p99 latency |
   | ------------------------------ | ----- | ----------- |
   | Single-thread                  | 2479  | 5.48ms      |
   | Client-per-thread (10 threads) | 2761  | 5.33ms      |
   | kqueue (1 worker)              | 1581  | 8.89ms      |
   | kqueue (10 workers)            | 6788  | 16.34ms     |

   Because the kqueue with 10 workers has scheduling overhead per individual request (100+ connections and only 10 workers with 10 clients each), the p99 is far larger, but gets a lot of them done hence the higher requests per second.

2. **Test 2:** Worker starvation: send `/compute` requests in the background (fibonacci between 40-46, stalling ~0.25-4s per call) while simultaneously sending `/health` requests in the foreground, to measure how much each architecture degrades under CPU-bound load.

   ```
   wrk -t2 -c10 -d10s --latency http://localhost:8080/compute &
   sleep 0.3
   wrk -t4 -c50 -d10s --latency http://localhost:8080/health
   wait
   ```

   **Results**

   | Architecture                   | Health req/s | Health p99 | Compute req/s | Compute p99 |
   | ------------------------------ | ------------ | ---------- | ------------- | ----------- |
   | Single-thread                  | 0            | N/A        | 0.8           | 1.35s       |
   | Client-per-thread (10 threads) | 104.9        | 457.92ms   | 5.0           | 2.00s       |
   | kqueue (1 worker)              | 4.8          | N/A        | 1.4           | 1.40s       |
   | kqueue (10 workers)            | 45.2         | 1.75s      | 7.3           | 1.32s       |

   Looking at the client-per-thread `/health` latency: p50 is 0.97ms but p99 spikes to 457ms. So most requests get through quickly, but some get stuck waiting behind a compute-blocked thread. `kqueue-10-workers` has a worse `/health` p99 (1.75s) but distributes load more evenly across all workers. 

### Visual of Results:

![diagram](/images/benchmark_results.png)

`kqueue` with 10 workers dominates raw throughput, nearly 2.5x faster than client-per-thread at `/health`. However, under CPU-bound starvation (Test 2), client-per-thread actually handles health requests better (104.9 vs 45.2 req/s). This makes sense: when all threads are blocked on CPU work, the event-loop offers no advantage. Idle threads in the pool can immediately pick up `/health` requests, while kqueue workers are equally stuck computing fibonacci.

**Limitations**

Although I was pleased with the performance results, I also wanted to explore the trade-offs. One limitation is the lack of connection reuse and keep-alive. Each response closes the connection, which causes the OS to hold the socket tuple in a TIME_WAIT state for ~60 seconds to guard against delayed packets. Under heavy load, thousands of these tuples accumulate, eventually exhausting the available port space and preventing new connections from being established. Implementing keep-alive, which reuses a connection across multiple requests, would eliminate this bottleneck.
