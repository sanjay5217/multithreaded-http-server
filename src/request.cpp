#include "request.hpp"

httpRequest::Request(string_dict info) {
    this->method = info["method"];
    this->path = info["path"];
    this->version = info["version"];
    this->headers = info["headers"];
    this->body = "";
};

void httpRequest::execute_handler(void) {

};

bool extract_body(Socket socket) {

};

std::string request_string(void) {

};

httpRequest::~Request() {

};