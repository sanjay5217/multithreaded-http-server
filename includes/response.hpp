#pragma once 

#include <string>
#include "request.hpp"

class httpResponse {
    public: 
    /**
     * @brief Constructs the Response object
     *
     * @param req: the request to be 
     * @return true if the handler has been executed and false otherwise
     */
    httpResponse(void);

    void add_status(std::string status);
    void add_header(std::string header);
    void add_body(std::string body);
    std::string finish_res(void);

    protected:
    std::string status_line;
    std::string body;
    std::string headers;
};