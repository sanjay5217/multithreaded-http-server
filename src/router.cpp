#include "../includes/router.hpp"
#include <iostream>

function_dict HANDLERS = {
    {"GET", {
        {"/health", health_handler}
    }}
};

std::string health_handler(const httpRequest& req) {
    httpResponse res{};
    res.add_status(req.get_version() + " 200 OK");
    for (const std::pair<const std::string, std::string>& pair: req.get_header()) {
        res.add_header(pair.first + ": " + pair.second);
    };
    std::string r_string = res.finish_res();
    return r_string;
}

Router::Router() {
    this->handlers = HANDLERS;
}

std::string Router::exec_handler(httpRequest& req) {
    auto method_type = this->handlers.find(req.get_method());
    if (method_type == this->handlers.end()) {
        return "";
    };   

    auto& inner_map = method_type->second;
    auto path = inner_map.find(req.get_path());
    if (path == inner_map.end()) {
        // craft response
        return "";
    };

    Handler& handle = path->second;
    return handle(req);
}

Router::~Router() {
}
