#pragma once 

#include <string>
#include <unordered_map>
#include "socket.hpp"


typedef std::unordered_map<std::string, std::string> string_dict;

class httpRequest {

    /*  curl Format: 
     * 
     *  METHOD PATH VERSION
     *  Header: Value
     *  Header: Value
     *  Header: Value
     * 
     *  Body (optional)
    */

    protected:
    std::string method;
    std::string path;
    std::string version;

    std:unordered_map<> headers;
    std:string body;

    public:
    Request(string_dict info);
    bool extract_body(Socket socket);
    void execute_handler(void);
    std::string request_string(void);
    ~Request();
};