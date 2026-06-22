#pragma once

#include <fstream>
#include <iostream>
#include <chrono>
#include <atomic>

#include "request.hpp"
#include "response.hpp"
#include "utils.hpp"

class Handler {
    protected:
        static std::atomic<int> num_req;

    public:
        Handler(void);
        virtual ~Handler() = default;

        /**
         * @brief Processes the request and returns a response.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        virtual httpResponse handle(httpRequest &req);
};

class HealthHandler : public Handler {
    public:
        HealthHandler(void);

        /**
         * @brief Returns 200 OK with a status body.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;
};

class StatHandler : public Handler {
    public:
        StatHandler(void);

        /**
         * @brief Returns the total number of requests served.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;
};

class EchoHandler : public Handler {
    public:
        EchoHandler(void);

        /**
         * @brief Echoes the request body back in the response.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;
};

class HeaderHandler : public Handler {
    public:
        HeaderHandler(void);

        /**
         * @brief Echoes request headers back as response headers.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;
};

class ComputeHandler : public Handler {
    public:
        ComputeHandler();

        /**
         * @brief Computes a Fibonacci number and returns the result with timing.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;
};

class StaticHandler : public Handler {
    public:
        StaticHandler();

        /**
         * @brief Serves a file whose path is given in the request body.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;

    protected:
        std::string file_name;
};

class InvalidHandler : public Handler {
    public:
        /**
         * @brief Constructs an error handler with a fixed status and message.
         *
         * @param status HTTP status code.
         * @param msg    Status message.
         */
        InvalidHandler(int status, std::string msg);

        /**
         * @brief Returns the configured error response.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        httpResponse handle(httpRequest &req) override;

    protected:
        std::string status;
        std::string msg;
};
