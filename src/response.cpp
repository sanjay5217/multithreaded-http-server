#include "../include/response.hpp"

httpResponse::httpResponse() : 
    status_line{}, body{},  headers{}{}

void httpResponse::add_status(std::string status) {this->status_line += status + "\r\n";}
void httpResponse::add_header(std::string header) {this->headers += header + "\r\n";}
void httpResponse::add_body(std::string body) {this->body += body + "\r\n";}

std::string httpResponse::finish_res() {
    this->headers += "\r\n";
    std::string res;
    res = this->status_line + this->headers + this->body;
    return res;
}
