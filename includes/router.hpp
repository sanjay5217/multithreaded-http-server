#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>

#include "request.hpp"
#include "response.hpp"
#include "handles.hpp"
#include "utils.hpp"

using function_dict = std::unordered_map<
    std::string,
    std::unordered_map<std::string, Handler*>
>;

extern function_dict HANDLERS;

class Router {
    private:
    function_dict handlers;


    public:

    /**
     * @brief Constructs the router
     */
    Router(void);

    /**
     * @brief executes the handler based on request
     *
     * @param req: the request to be executed
     * @return true if the handler has been executed and false otherwise
     */
    httpResponse exec_handler(httpRequest &req);
};