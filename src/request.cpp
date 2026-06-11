#include "../includes/request.hpp"

httpRequest::httpRequest(std::string method, std::string path, std::string version) {
    this->method = method;
    this->path = path;
    this->version = version;
    this->headers = {};
    this->body = "";
};

std::string httpRequest::get_method() const {return this->method;}

std::string httpRequest::get_path() const {return this->path;}

std::string httpRequest::get_version() const {return this->version;}

string_dict httpRequest::get_header() const {return this->headers;}

void httpRequest::fill_headers(string_dict info) {
    for (const std::pair<const std::string, std::string>& header_pair: info) {
        this->headers[header_pair.first] = header_pair.second;
    }
}

void httpRequest::set_body(std::string body) { this->body = body; }

bool httpRequest::extract_body() {
    return true;
};

httpRequest::~httpRequest() {
};