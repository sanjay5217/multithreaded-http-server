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
         * @brief Constructs the router.
         */
        Router(void);

        /**
         * @brief Routes the request to the appropriate router.
         *
         * @param req The HTTP request to be processed.
         * @return The HTTP response in accordance with the request.
         */
        httpResponse exec_handler(httpRequest &req);

        ~Router();
};
