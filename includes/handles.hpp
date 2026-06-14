#pragma once

#include <fstream>
#include <iostream>
#include <chrono>

#include "request.hpp"
#include "response.hpp"
#include "utils.hpp"

class Handler {
    protected:
        static int num_req;

    public:
        /**
         * @brief Constructs the base handler.
         */
        Handler(void);

        /**
         * @brief Processes the request and returns a response.
         *
         * @param req The incoming HTTP request.
         * @return The HTTP response.
         */
        virtual httpResponse handle(httpRequest &req);

        virtual ~Handler() = default;
};

class HealthHandler : public Handler {
    public:
        /**
         * @brief Constructs a HealthHandler.
         */
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
        /**
         * @brief Constructs a StatHandler.
         */
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
        /**
         * @brief Constructs an EchoHandler.
         */
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
        /**
         * @brief Constructs a HeaderHandler.
         */
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
        /**
         * @brief Constructs a ComputeHandler.
         */
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
        /**
         * @brief Constructs a StaticHandler.
         */
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
    protected:
        std::string status;
        std::string msg;

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
};
