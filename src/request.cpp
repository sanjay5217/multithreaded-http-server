#include "../include/request.hpp"

httpRequest::httpRequest(std::string method, std::string path, std::string version) : 
    method{method}, 
    path{path}, 
    version{version}, 
    headers{}, 
    body{} {};

std::string httpRequest::get_method() const {return this->method;}

std::string httpRequest::get_path() const {return this->path;}

std::string httpRequest::get_version() const {return this->version;}

string_dict httpRequest::get_header() const {return this->headers;}

std::string httpRequest::get_body() const {return this->body;}

void httpRequest::fill_headers(string_dict info) {
    for (const std::pair<const std::string, std::string>& header_pair: info) {
        this->headers[header_pair.first] = header_pair.second; }
}

void httpRequest::set_body(std::string body) {this->body = body;}

bool httpRequest::set_length(int length) {
    if (this->headers.find("Content-Length") == this->headers.end()) {
        return false;
    }
    this->headers["Content-Length"] = std::to_string(length);
    return true;    
}
