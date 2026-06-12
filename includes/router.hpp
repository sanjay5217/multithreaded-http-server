#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>

#include "request.hpp"
#include "response.hpp"

using Handler = std::function<std::string(const httpRequest&)>;

using function_dict = std::unordered_map<
    std::string,
    std::unordered_map<std::string, Handler>
>;

// HANDLERS
std::string invalid_handler(const httpRequest& req, int status, std::string msg);
std::string health_handler(const httpRequest &req);
std::string stat_handler(const httpRequest &req);
std::string echo_handler(const httpRequest &req);
std::string header_handler(const httpRequest &req);


extern function_dict HANDLERS;

class Router {
    private:
    function_dict handlers;
    int count;

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
    std::string exec_handler(httpRequest &req);

    /**
     * @brief Deconstructor
     */
    ~Router(void);
};