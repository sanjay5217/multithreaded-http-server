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
    httpRequest(std::string, std::string, std::string);

    /**
     * @return the method of the request
     */
    std::string get_method(void) const;

    /**
     * @return the path of the request
     */
    std::string get_path(void) const;
    std::string get_version(void) const;
    string_dict get_header(void) const;
    std::string get_body(void) const;
    void fill_headers(string_dict info);
    void set_body(std::string body);
    //std::string extract_body(const Socket &socket);
};
