#include "../include/handles.hpp"
#include "../include/utils.hpp"

std::atomic<int> Handler::num_req = 0;

Handler::Handler() {}

httpResponse Handler::handle(httpRequest &req) {
    httpResponse res{};
    res.add_status(req.get_version() + " 200 OK");
        for (const std::pair<const std::string, std::string>& pair: req.get_header()) {
            // Bug: blindly echoing the request's Content-Length describes the
            // *request* body size, not the response body about to be written.
            // A spec-compliant client trusts Content-Length to know when the
            // response ends, so forwarding the request's value corrupts
            // response framing (truncated reads, or the client hanging for
            // bytes that never arrive). Fix: skip it here; each handler's own
            // add_body() calls define the actual response framing.
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
    // Bug: pop_back() on an empty string is undefined behavior. A request
    // with zero headers (e.g. raw "GET /header HTTP/1.1\r\n\r\n") left
    // header_body empty and crashed the worker thread. Fix: only trim the
    // trailing newline when there's something to trim.
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
    // Bug: the 3-iterator std::mismatch only bounds-checks `base`'s range; it
    // keeps advancing `target`'s iterator in lockstep without checking it
    // against target.end(). Any existing file whose canonical path has fewer
    // components than the server's cwd (e.g. requesting "/etc/hosts" while
    // running deep in a project directory) makes it walk past target.end(),
    // reading out-of-bounds path-component iterators (ASan: heap-buffer-overflow).
    // Fix: use the 4-iterator overload, which stops at the shorter range.
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





