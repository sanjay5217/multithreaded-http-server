#pragma once

#include <fstream>
#include <iostream>
#include <chrono>

#include "request.hpp"
#include "response.hpp"
#include "utils.hpp"

class Handler {
    public:
    Handler(void);
    virtual httpResponse handle(httpRequest &req);
    virtual ~Handler() = default;

    protected:
    static int num_req;
};

class HealthHandler : public Handler {
    public:
    HealthHandler(void);
    httpResponse handle(httpRequest &req) override;
};

class StatHandler : public Handler {
    public:
    StatHandler(void);
    httpResponse handle(httpRequest &req) override;
};

class EchoHandler : public Handler {
    public:
    EchoHandler(void);
    httpResponse handle(httpRequest &req) override;
};

class HeaderHandler : public Handler {
    public:
    HeaderHandler(void);
    httpResponse handle(httpRequest &req) override;
};

class ComputeHandler : public Handler {
    public:
    ComputeHandler();
    httpResponse handle(httpRequest &req) override;
};

class StaticHandler : public Handler {
    public:
    StaticHandler();
    httpResponse handle(httpRequest &req) override;

    protected:
    std::string file_name;
};

class InvalidHandler : public Handler {
    public:
    InvalidHandler(int status, std::string msg);
    httpResponse handle(httpRequest &req) override;
    protected:
    std::string status;
    std::string msg;
};