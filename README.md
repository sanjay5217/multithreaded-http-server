# HTTP-Server

A multithreaded HTTP/1.1 server built from scratch in C++17, using raw POSIX sockets with no external networking libraries. This project was a deep dive into C++ OOP, RAII resource management, design principles, systems programming including sockets and http parsing, and finally concurrent request handling via a thread pool and job queue.
 
I divided this project into 2 phases.
 
- **Phase 1:** Core HTTP parsing pipeline with back-to-back single-threaded request handling, covering six custom endpoints:
- **Phase 2:** Thread pool, shared state, graceful shutdown, CMake build system, and benchmarking with `wrk`

## Phase 1

**Phase 1 Blueprint:**

![diagram](/images/image-1.png)

Phase 1 is a single-thread server that just parses the http send by `curl localhost:PORT` and replies back. The main objective was to get the tcp socket working by being able to read and parse raw HTTP bytes and send messages back. To make this task much more complex, I decided to create 6 unique endpoints to challenge myself with the design of the server. 

*   `GET /health`: returns the status of the server

    ```
    $ curl http://localhost:8080/health
    { Status: OK }
    ```

*   `GET /stat`: returns the number of requests served

    ```
    curl http://localhost:8080/stat 
    { Requests Served: 2 }
    ```

*   `GET /header`: returns the headers of the request

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

*   `GET /compute`: returns the cpu time of the server using a fibonnaci calculation

    ```
    curl http://localhost:8080/compute                  
    { N: 44, Result: 1134903170, Duration: 3.34752 }
    ```

*   `POST /echo`: returns the message written (similar to bash command `echo`)

    ```
    curl http://localhost:8080/echo -d "Hello World"
    Hello World
    ```

*   `POST/static`: returns the file output (similar to bash command `cat`)

    ```
    curl http://localhost:8080/static -d input.txt
    Hello world!
    This is a test file.
    Used for StaticHandler testing.
    End of file.
    ```

    *Note: The directory is relative to whichever server directory `./server` is run on.*


To do this, I constructed a Router class that consisted of the method `exec_handler`. This followed the Command design pattern, where I constructed handler classes and used isomorphism to return a httpResponse depending on the path. 

![diagram](/images/image-2.png)

## Phase 2

Phase 2 introduces concurrency to address two inefficiencies in the single-threaded server:

1. Requests are handled sequentially right now. This means a slow or expensive request can block every client behind it.
2. The server uses blocking I/O; `recv()` freezes the thread until bytes arrive, wasting CPU time on slow senders.

**Version 1 — Thread Pool**

To address the first problem, I refactored the server to use a fixed thread pool with a condition-variable-driven job queue. At startup, N worker threads are spawned and sleep on a condition variable. When the main thread accepts a client fd and pushes it onto the queue, one worker wakes up, handles the full request, and returns to sleep. Requests are now processed concurrently, so a slow request like `/compute` no longer blocks `/health`. An abstract model highlights the client-per-thread model. 

![diagram](/images/image-3.png)