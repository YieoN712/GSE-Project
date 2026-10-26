#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "MonitorServer.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

struct MonitorServer::Implementation
{
    SOCKET listener = INVALID_SOCKET;
    bool winsock = false;
    std::atomic<bool> running{false};
    std::thread worker;
    std::mutex mutex;
    std::string snapshot = "{\"schemaVersion\":1,\"ready\":false}";
    std::string html;

    void Respond(SOCKET client)
    {
        DWORD timeout = 100;
        setsockopt(
            client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
        setsockopt(
            client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
        std::string request;
        char buffer[2048];
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
        while (running && request.size() < 8192 && request.find("\r\n\r\n") == std::string::npos)
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                return;
            }
            const int received = recv(client, buffer, sizeof(buffer), 0);
            if (received <= 0)
            {
                return;
            }
            request.append(buffer, received);
        }
        if (request.find("\r\n\r\n") == std::string::npos)
        {
            return;
        }
        std::istringstream lines(request);
        std::string method, path, protocol;
        lines >> method >> path >> protocol;
        std::string line;
        // Consume the request-line remainder before checking for the header terminator.
        std::getline(lines, line);
        bool localHost = false;
        while (std::getline(lines, line))
        {
            // Browsers use this exact host header; reject alternate origins / DNS rebinding.
            if (line == "Host: 127.0.0.1:8765\r" || line == "Host: localhost:8765\r")
            {
                localHost = true;
            }
            if (line == "\r")
            {
                break;
            }
        }
        std::string body;
        std::string status = "200 OK";
        std::string type = "application/json; charset=utf-8";
        if (!localHost || method != "GET")
        {
            status = "403 Forbidden";
            body = "{}";
        }
        else if (path == "/api/snapshot")
        {
            std::lock_guard<std::mutex> lock(mutex);
            body = snapshot;
        }
        else if (path == "/" || path == "/index.html")
        {
            type = "text/html; charset=utf-8";
            body = html;
        }
        else
        {
            status = "404 Not Found";
            body = "{}";
        }
        const std::string response =
            "HTTP/1.1 " + status + "\r\nContent-Type: " + type +
            "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff"
            "\r\nContent-Security-Policy: default-src 'self'; script-src 'unsafe-inline'; "
            "style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'"
            "\r\nConnection: close\r\nContent-Length: " +
            std::to_string(body.size()) + "\r\n\r\n" + body;
        size_t sent = 0;
        const auto sendDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (running && sent < response.size() && std::chrono::steady_clock::now() < sendDeadline)
        {
            const int count =
                send(client, response.data() + sent, static_cast<int>(response.size() - sent), 0);
            if (count <= 0)
            {
                break;
            }
            sent += count;
        }
    }

    void Run()
    {
        while (running)
        {
            fd_set ready;
            FD_ZERO(&ready);
            FD_SET(listener, &ready);
            timeval timeout{0, 100000};
            const int result = select(0, &ready, nullptr, nullptr, &timeout);
            if (result == SOCKET_ERROR)
            {
                break;
            }
            if (result > 0 && running)
            {
                SOCKET client = accept(listener, nullptr, nullptr);
                if (client != INVALID_SOCKET)
                {
                    try
                    {
                        Respond(client);
                    }
                    catch (const std::exception& error)
                    {
                        std::cerr << "Monitor request failed: " << error.what() << '\n';
                    }
                    closesocket(client);
                }
            }
        }
    }
};

MonitorServer::MonitorServer() : implementation(new Implementation())
{
}

MonitorServer::~MonitorServer()
{
    Stop();
}

bool MonitorServer::Start()
{
    Stop();
    auto& state = *implementation;
    wchar_t executable[32768];
    const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
    if (!length || length >= 32768)
    {
        return false;
    }
    std::ifstream input(std::filesystem::path(executable).parent_path() / L"monitor" /
                            L"index.html",
                        std::ios::binary);
    if (!input)
    {
        std::cerr << "Monitor HTML missing beside executable (monitor/index.html).\n";
        return false;
    }
    state.html.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        return false;
    }
    state.winsock = true;
    state.listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BOOL exclusive = TRUE;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(8765);
    if (state.listener == INVALID_SOCKET ||
        setsockopt(state.listener,
                   SOL_SOCKET,
                   SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<char*>(&exclusive),
                   sizeof(exclusive)) == SOCKET_ERROR ||
        bind(state.listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
            SOCKET_ERROR ||
        listen(state.listener, 8) == SOCKET_ERROR)
    {
        std::cerr << "Monitor unavailable on 127.0.0.1:8765, Winsock error " << WSAGetLastError()
                  << '\n';
        Stop();
        return false;
    }
    state.running = true;
    try
    {
        state.worker = std::thread(
            [&state]()
            {
                state.Run();
            });
    }
    catch (...)
    {
        Stop();
        throw;
    }
    std::cout << "Monitor: http://127.0.0.1:8765/ (read-only, local PC only)\n";
    return true;
}

void MonitorServer::Stop()
{
    auto& state = *implementation;
    state.running = false;
    if (state.worker.joinable())
    {
        state.worker.join();
    }
    if (state.listener != INVALID_SOCKET)
    {
        closesocket(state.listener);
        state.listener = INVALID_SOCKET;
    }
    if (state.winsock)
    {
        WSACleanup();
        state.winsock = false;
    }
}

void MonitorServer::Publish(std::string snapshot)
{
    std::lock_guard<std::mutex> lock(implementation->mutex);
    implementation->snapshot = std::move(snapshot);
}
