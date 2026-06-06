#include "rest_api.hpp"
#include "../core/protocol_router.hpp"
#include "../database/db_manager.hpp"
#include "../manufacturers/base.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace ecumult {

RESTAPI::RESTAPI(std::shared_ptr<ProtocolRouter> router,
                 std::shared_ptr<DBManager> db,
                 const std::string& host, uint16_t port)
    : router_(std::move(router))
    , db_(std::move(db))
    , host_(host)
    , port_(port)
    , running_(false) {}

RESTAPI::~RESTAPI() {
    stop();
}

bool RESTAPI::start() {
    if (running_) return true;
    running_ = true;
    server_thread_ = std::thread(&RESTAPI::apiLoop, this);
    std::cout << "[API] REST server starting on " << host_ << ":" << port_ << std::endl;
    return true;
}

void RESTAPI::stop() {
    running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
    std::cout << "[API] REST server stopped" << std::endl;
}

bool RESTAPI::isRunning() const {
    return running_;
}

void RESTAPI::setRouteHandler(RouteHandler handler) {
    custom_handler_ = std::move(handler);
}

void RESTAPI::apiLoop() {
    // Simple TCP-based HTTP server (for Crow/oat++ replacement)
    // In production, replace with Crow or uWebSockets

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[API] Socket creation failed" << std::endl;
        return;
    }

    int opt = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[API] Bind failed on port " << port_ << std::endl;
        ::close(server_fd);
        return;
    }

    ::listen(server_fd, 10);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ::setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::cout << "[API] HTTP server listening on " << port_ << std::endl;

    while (running_) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            break;
        }

        // Simple HTTP response for all requests
        std::string response_body = R"({"status":"ok","service":"ECU Multi-Emulator","version":"1.0.0"})";
        std::string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Content-Length: " + std::to_string(response_body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response_body;

        ::write(client_fd, http_response.c_str(), http_response.size());
        ::close(client_fd);
    }

    ::close(server_fd);
}

} // namespace ecumult
