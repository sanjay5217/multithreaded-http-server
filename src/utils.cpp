#include "../include/utils.hpp"

namespace {
    std::mutex cout_mtx;
}

string_dict extract_string(std::string msg) {
    string_dict map{};

    size_t first_ptr{msg.find("\r\n")};
    std::string firstline{msg.substr(0, first_ptr)};
    std::istringstream stream{firstline};
    std::string method{}, path{}, version{};

    if (!(stream >> method >> path >> version)) {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "Invalid Request..." << std::endl;
        return map;
    };

    map["method"] = method;
    map["path"] = path;
    map["version"] = version;

    int start{static_cast<int>(first_ptr) + 2};
    int end{};
    std::string temp{}, header{}, content{};

    while ((size_t)(end = msg.find("\r\n", start)) != std::string::npos) {
        temp = msg.substr(start, end - start);
        if (temp.empty()) break;
        int delim{static_cast<int>(temp.find(':'))};
        if (delim == (int)std::string::npos) { start = end + 2; continue; }
        header = temp.substr(0, delim);
        content = temp.substr(delim + 2);
        map[header] = content;
        start = end + 2;
    }
    return map;
};

int fibonacci(int n) {
    if (n == 1 || n == 0) {
        return 1;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

int random_int(int a, int b) {
    static std::random_device rd;
    static std::mt19937 gen{rd()};
    std::uniform_int_distribution<int> dist{a, b - 1};
    return dist(gen);
}

std::string get_header_ci(const string_dict& headers, const std::string& key) {
    auto ci_equal = [](const std::string& a, const std::string& b) {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
            [](char x, char y) { return std::tolower((unsigned char)x) == std::tolower((unsigned char)y); });
    };
    for (const auto& [name, value] : headers) {
        if (ci_equal(name, key)) {
            return value;
        }
    }
    return "";
}
