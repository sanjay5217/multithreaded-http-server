#include "../includes/handles.hpp"

int Handler::num_req = 0;

Handler::Handler() {}

httpResponse Handler::handle(const httpRequest &req) {
    httpResponse res{};
    res.add_status(req.get_version() + " 200 OK");
        for (const std::pair<const std::string, std::string>& pair: req.get_header()) {
            res.add_header(pair.first + ": " + pair.second);
    };
    this->num_req+=1;
    return res;
}

HealthHandler::HealthHandler() : Handler() {}

httpResponse HealthHandler::handle(const httpRequest &req) {
    httpResponse res = Handler::handle(req);
    res.add_body("status ok");
    return res;
}

StatHandler::StatHandler() : Handler() {;}

httpResponse StatHandler::handle(const httpRequest &req) {
    httpResponse res = Handler::handle(req);
    res.add_body("{ Requests Served: " + std::to_string(this->num_req) + "}");
    return res;
}

EchoHandler::EchoHandler() : Handler() {}

httpResponse EchoHandler::handle(const httpRequest &req) {
    httpResponse res = Handler::handle(req);
    res.add_body(req.get_body());
    return res;
}

HeaderHandler::HeaderHandler() : Handler() {}

httpResponse HeaderHandler::handle(const httpRequest &req) {
    return Handler::handle(req);
}

InvalidHandler::InvalidHandler(int status, std::string msg)
    : Handler() {
        this->status = std::to_string(status);
        this->msg = msg;
}

httpResponse InvalidHandler::handle(const httpRequest &req) {
    httpResponse res{};
    res.add_status(req.get_version() + " " + this->status + " " + this->msg);
    res.add_body(msg);
    return res;
}





