#pragma once 

#include <string>
#include <unordered_map>
#include "socket.hpp"


typedef std::unordered_map<std::string, std::string> string_dict;

class httpRequest {
    protected:
        std::string method; 
        std::string path;
        std::string version;
        string_dict headers;
        std::string body;

    public:
        /**
         * @brief Constructs a request.
         *
         * @param method HTTP method (e.g., GET, POST).
         * @param path   Request path.
         * @param version HTTP version string.
         */
        httpRequest(std::string method, std::string path, std::string version);

        /**
         * @return The HTTP method.
         */
        std::string get_method(void) const;

        /**
         * @return The request path.
         */
        std::string get_path(void) const;

        /**
         * @return The HTTP version string.
         */
        std::string get_version(void) const;

        /**
         * @return The request headers as a key-value map.
         */
        string_dict get_header(void) const;

        /**
         * @return The request body.
         */
        std::string get_body(void) const;

        /**
         * @brief Populates headers from a parsed key-value map.
         *
         * @param info Map of header names to values.
         */
        void fill_headers(string_dict info);

        /**
         * @brief Sets the request body.
         *
         * @param body Body content.
         */
        void set_body(std::string body);

        /**
         * @brief Sets the Content-Length header.
         *
         * @param length Length of the body in bytes.
         * @return True if set successfully, false otherwise.
         */
        bool set_length(int length);
};
