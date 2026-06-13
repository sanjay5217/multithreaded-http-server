#include "../includes/router.hpp"
#include "../includes/handles.hpp"
#include "../includes/utils.hpp"

function_dict HANDLERS = {
    {"GET", {
        {"/health", new HealthHandler()},
        {"/stat", new StatHandler()},
        {"/header", new HeaderHandler()}
    }},
    {"POST", {
        {"/echo", new EchoHandler()}
    }}
};

Router::Router() {
    this->handlers = std::move(HANDLERS);
    this->count = 0;
}

httpResponse Router::exec_handler(httpRequest& req) {
    auto method_type = this->handlers.find(req.get_method());
    if (method_type == this->handlers.end()) {
        return InvalidHandler(400, "Invalid Method").handle(req);
    }

    auto& inner_map = method_type->second;
    auto path = inner_map.find(req.get_path());
    if (path == inner_map.end()) {
        return InvalidHandler(401, "Invalid Path").handle(req);
    }

    return path->second->handle(req);
};
