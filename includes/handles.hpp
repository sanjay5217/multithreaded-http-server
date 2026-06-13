#pragma once

#include "request.hpp"
#include "response.hpp"
#include "utils.hpp"

class Handler {
    public:
    Handler(void);
    virtual httpResponse handle(const httpRequest &req);
    virtual ~Handler() = default;

    protected:
    static int num_req;
};

class HealthHandler : public Handler {
    public:
    HealthHandler(void);
    httpResponse handle(const httpRequest &req) override;
};

class StatHandler : public Handler {
    public:
    StatHandler(void);
    httpResponse handle(const httpRequest &req) override;
};

class EchoHandler : public Handler {
    public:
    EchoHandler(void);
    httpResponse handle(const httpRequest &req) override;
};

class HeaderHandler : public Handler {
    public:
    HeaderHandler(void);
    httpResponse handle(const httpRequest &req) override;
};

// class ComputeHandler : public Handler {
//     public:
//     HealthHandler();
//     httpResponse handle(const httpRequest &req) override;
// };

// class StaticHandler : public Handler {
//     public:
//     HealthHandler();
//     httpResponse handle(const httpRequest &req) override;
// };

class InvalidHandler : public Handler {
    public:
    InvalidHandler(int status, std::string msg);
    httpResponse handle(const httpRequest &req) override;
    protected:
    std::string status;
    std::string msg;
};