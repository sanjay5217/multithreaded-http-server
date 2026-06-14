#include "../includes/router.hpp"
#include "../includes/handles.hpp"
#include "../includes/utils.hpp"

function_dict HANDLERS{
    {"GET", {
        {"/health", new HealthHandler()},
        {"/stat", new StatHandler()},
        {"/header", new HeaderHandler()}, 
        {"/compute", new ComputeHandler()},
    }},
    {"POST", {
        {"/echo", new EchoHandler()}, 
        {"/static", new StaticHandler()}
    }}
};

Router::Router() : 
    handlers{std::move(HANDLERS)} {} //, count{0} 

httpResponse Router::exec_handler(httpRequest& req) {
    auto method_type = this->handlers.find(req.get_method());
    if (method_type == this->handlers.end()) {
        return InvalidHandler{405, "Method Not Allowed"}.handle(req);
    }

    auto& inner_map = method_type->second;
    auto path = inner_map.find(req.get_path());
    if (path == inner_map.end()) {
        return InvalidHandler{404, "Not Found"}.handle(req);
    }
    return path->second->handle(req);
};

Router::~Router() {
    for (auto& [method, paths] : this->handlers) {
        for (auto& [path, handler] : paths) {
            delete handler;
        }
    }
}