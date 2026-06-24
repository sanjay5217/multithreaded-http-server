#include "../include/handles.hpp"
#include "../include/utils.hpp"

std::atomic<int> Handler::num_req = 0;

Handler::Handler() {}

httpResponse Handler::handle(httpRequest &req) {
    httpResponse res{};
    res.add_status(req.get_version() + " 200 OK");
        for (const std::pair<const std::string, std::string>& pair: req.get_header()) {
            std::string name{pair.first};
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char c) { return std::tolower(c); });
            if (name == "content-length") { continue; }
            res.add_header(pair.first + ": " + pair.second);
    };
    this->num_req+=1;
    return res;
}

HealthHandler::HealthHandler() : Handler{} {}

httpResponse HealthHandler::handle(httpRequest &req) {
    httpResponse res{Handler::handle(req)};
    res.add_body("{ Status: OK }");
    return res;
}

StatHandler::StatHandler() : Handler{} {}

httpResponse StatHandler::handle(httpRequest &req) {
    httpResponse res{Handler::handle(req)};
    res.add_body("{ Requests Served: " + std::to_string(this->num_req) + " }");
    return res;
}

EchoHandler::EchoHandler() : Handler{} {}

httpResponse EchoHandler::handle(httpRequest &req) {
    httpResponse res{Handler::handle(req)};
    res.add_body(req.get_body());
    return res;
}

HeaderHandler::HeaderHandler() : Handler{} {}

httpResponse HeaderHandler::handle(httpRequest &req) {
    httpResponse res{Handler::handle(req)};
    string_dict headers_map{req.get_header()};
    std::string header_body{};
    for (const std::pair<const std::string, std::string>& header_pair: headers_map) {
        std::stringstream ss{};
        ss << "{ " << header_pair.first << ": " << header_pair.second << " }\n";
        header_body += ss.str();
    }
    if (!header_body.empty()) { header_body.pop_back(); }
    res.add_body(header_body);
    return res;
}

ComputeHandler::ComputeHandler() : Handler{} {}

httpResponse ComputeHandler::handle(httpRequest &req) {
    httpResponse res{Handler::handle(req)};
    int n{random_int(20, 26)};
    auto start{std::chrono::high_resolution_clock::now()};
    int result{fibonacci(n)};
    auto end{std::chrono::high_resolution_clock::now()};
    std::chrono::duration<float> duration{end - start};

    std::stringstream ss{};
    ss << "{ N: " << n 
        << ", Result: " 
        << result 
        << ", Duration: " 
        << duration.count() 
        << " }";

    res.add_body(ss.str());
    return res;
}

StaticHandler::StaticHandler() : Handler{}, 
    file_name{} {}

httpResponse StaticHandler::handle(httpRequest &req) {
    namespace fs = std::filesystem;

    std::string file_name{req.get_body()};

    std::error_code ec;
    if (!fs::is_regular_file(file_name, ec)) {
        return InvalidHandler{404, "Not Found"}.handle(req);
    }

    fs::path base{fs::current_path()};
    fs::path target{fs::weakly_canonical(file_name, ec)};
    auto [b, t]{std::mismatch(base.begin(), base.end(), target.begin(), target.end())};
    if (b != base.end()) {
        return InvalidHandler{403, "Forbidden"}.handle(req);
    }

    std::ifstream inp { file_name };
    if (!inp) {
        return InvalidHandler{404, "Not Found"}.handle(req);
    }
    std::string final_input{
        std::istreambuf_iterator<char>(inp),
        std::istreambuf_iterator<char>()
    };
    req.set_length(final_input.length());
    httpResponse res{Handler::handle(req)};
    res.add_body(final_input);
    return res;
}

InvalidHandler::InvalidHandler(int status, std::string msg) : Handler{},
    status{std::to_string(status)},
    msg{msg} {}

httpResponse InvalidHandler::handle(httpRequest &req) {
    httpResponse res{};
    res.add_status(req.get_version() + " " + this->status + " " + this->msg);
    res.add_body(this->msg);
    res.add_body("Status " + this->status + " NOT OK");
    return res;
}





