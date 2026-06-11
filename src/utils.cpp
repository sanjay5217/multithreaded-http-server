#include "../includes/utils.hpp"

string_dict extract_string(std::string msg) {
    string_dict map = {};

    size_t first_ptr = msg.find("\r\n");
    std::string firstline = msg.substr(0, first_ptr);
    std::istringstream stream(firstline);
    std::string method, path, version;

    if (!(stream >> method >> path >> version)) {
        std::cout << "Invalid Request..." << std::endl; 
        return map;
    };

    map["method"] = method;
    map["path"] = path;
    map["version"] = version;

    int start = first_ptr + 2;
    int end;
    std::string temp, header, content;

    while ((size_t)(end = msg.find("\r\n", start)) != std::string::npos) {
        temp = msg.substr(start, end - start);
        if (temp.empty()) break;
        int delim = temp.find(':');
        if (delim == (int)std::string::npos) { start = end + 2; continue; }
        header = temp.substr(0, delim);
        content = temp.substr(delim + 2);
        // std::transform(header.begin(), header.end(), header.begin(), [](unsigned char c) {
        //     return std::tolower(c);
        // });
        map[header] = content;
        start = end + 2;
    }
    return map;
};