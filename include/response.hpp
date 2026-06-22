#pragma once 

#include <string>
#include "request.hpp"

class httpResponse {
    protected:
        std::string status_line;
        std::string body;
        std::string headers;

    public:
        /**
         * @brief Constructs an empty response.
         */
        httpResponse(void);

        /**
         * @brief Sets the status line.
         *
         * @param status Full status line (e.g., "HTTP/1.1 200 OK").
         */
        void add_status(std::string status);

        /**
         * @brief Appends a header field.
         *
         * @param header Header string (e.g., "Content-Type: text/plain").
         */
        void add_header(std::string header);

        /**
         * @brief Appends to the response body.
         *
         * @param body Content to append.
         */
        void add_body(std::string body);

        /**
         * @brief Serializes the response into a raw HTTP string.
         *
         * @return The complete HTTP response.
         */
        std::string finish_res(void);
};
