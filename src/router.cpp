#include "../includes/router.hpp"

function_dict HANDLERS = {
    {"GET", {
        {"/health", health_handler}, 
        {"/stat", stat_handler},
        {"/header", header_handler}
    }}, 
    {"POST", {
        {"/echo", echo_handler}
    }}
};

httpResponse initialize_res(const httpRequest& req) {
    httpResponse res{};
    res.add_status(req.get_version() + " 200 OK");
        for (const std::pair<const std::string, std::string>& pair: req.get_header()) {
            if (pair.first != "Req-Count") {
                res.add_header(pair.first + ": " + pair.second);
            }
    };
    return res;
}

std::string invalid_handler(const httpRequest& req, int status, std::string msg) {
    httpResponse res{};
    res.add_status(req.get_version() + " " + std::to_string(status) + " " + msg);
    res.add_body(msg);
    std::string r_string = res.finish_res();
    return r_string;
}

std::string health_handler(const httpRequest& req) {
    httpResponse res = initialize_res(req);
    res.add_body("status ok");
    std::string r_string = res.finish_res();
    return r_string;
}

std::string echo_handler(const httpRequest& req) {
    httpResponse res = initialize_res(req);
    res.add_body(req.get_body());  
    std::string r_string = res.finish_res();
    return r_string;
}

std::string stat_handler(const httpRequest& req) {
    httpResponse res = initialize_res(req);
    res.add_body("Requests Served: " + req.get_header()["Req-Count"]);
    std::string r_string = res.finish_res();
    return r_string;
}

std::string header_handler(const httpRequest& req) {
    httpResponse res = initialize_res(req);
    std::string r_string = res.finish_res();
    return r_string;
}

Router::Router() {
    this->handlers = HANDLERS;
    this->count = 0;
}

std::string Router::exec_handler(httpRequest& req) {
    auto method_type = this->handlers.find(req.get_method());
    if (method_type == this->handlers.end()) {
        return invalid_handler(req, 400, "Invalid Method");
    };   

    auto& inner_map = method_type->second;
    auto path = inner_map.find(req.get_path());
    if (path == inner_map.end()) {
        return invalid_handler(req, 401, "Invalid Path");
    };

    this->count+=1;
    if (path->first == "/stat") {
        string_dict r_map = {{"Req-Count", std::to_string(this->count)}};
        req.fill_headers(r_map);
    }
    Handler& handle = path->second;
    return handle(req);
}

Router::~Router() {
}
